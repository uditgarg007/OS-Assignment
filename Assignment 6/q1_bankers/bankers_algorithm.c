/**
 * ============================================================================
 * Operating Systems Lab — Assignment 6
 * Question 1: Banker's Algorithm for Deadlock Avoidance
 *
 * Student Name   : Udit
 * Roll Number    : 2401MC07
 * Course         : MA3105 / Operating Systems Lab
 * ============================================================================
 *
 * Theoretical Background:
 * Dijkstra's Banker's Algorithm is a classic deadlock avoidance strategy
 * that tests for safety by simulating the allocation for predetermined maximum
 * possible amounts of all resources, and then makes an "s-state" check to test
 * for possible activities, before deciding whether allocation should be allowed.
 *
 * An allocation state is deemed SAFE if there exists a sequence of processes
 * <P_s1, P_s2, ..., P_sn> such that for each P_si, the resources that P_si can
 * still request can be satisfied by the currently available resources plus
 * the resources held by all preceding processes P_sj (j < i).
 * ============================================================================
 */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#define NUM_PROCESSES 5   /* Total number of active processes (P0 to P4) */
#define NUM_RESOURCES 3   /* Total number of distinct resource classes (A, B, C) */

/* Core Data Structure encapsulating the complete Banker's System State */
typedef struct {
    int allocation[NUM_PROCESSES][NUM_RESOURCES]; /* Currently allocated instances */
    int max_demand[NUM_PROCESSES][NUM_RESOURCES]; /* Maximum declared requirement */
    int need[NUM_PROCESSES][NUM_RESOURCES];       /* Remaining requirement (Max - Alloc) */
    int available[NUM_RESOURCES];                 /* Currently free resource instances */
} BankerSystem;

/**
 * Initialize system matrices with the assignment's baseline configuration.
 */
void banker_init(BankerSystem *sys) {
    /* Initial allocation matrix */
    const int init_alloc[NUM_PROCESSES][NUM_RESOURCES] = {
        {0, 1, 0},  /* P0 */
        {2, 0, 0},  /* P1 */
        {3, 0, 2},  /* P2 */
        {2, 1, 1},  /* P3 */
        {0, 0, 2}   /* P4 */
    };

    /* Maximum claim matrix */
    const int init_max[NUM_PROCESSES][NUM_RESOURCES] = {
        {7, 5, 3},  /* P0 */
        {3, 2, 2},  /* P1 */
        {9, 0, 2},  /* P2 */
        {2, 2, 2},  /* P3 */
        {4, 3, 3}   /* P4 */
    };

    /* Initial available vector: A=3, B=3, C=2 */
    const int init_avail[NUM_RESOURCES] = {3, 3, 2};

    memcpy(sys->allocation, init_alloc, sizeof(init_alloc));
    memcpy(sys->max_demand, init_max, sizeof(init_max));
    memcpy(sys->available, init_avail, sizeof(init_avail));

    /* Compute Need[i][j] = Max[i][j] - Alloc[i][j] */
    for (int p = 0; p < NUM_PROCESSES; p++) {
        for (int r = 0; r < NUM_RESOURCES; r++) {
            sys->need[p][r] = sys->max_demand[p][r] - sys->allocation[p][r];
        }
    }
}

/**
 * Pretty-print the complete current system state in a structured tabular format.
 */
void print_system_state(const BankerSystem *sys, const char *banner) {
    printf("\n+=====================================================================+\n");
    printf("| %-67s |\n", banner);
    printf("+---------+-------------------+-------------------+-------------------+\n");
    printf("| Process |    Allocation     |     Max Claim     |  Remaining Need   |\n");
    printf("|         |   A     B     C   |   A     B     C   |   A     B     C   |\n");
    printf("+---------+-------------------+-------------------+-------------------+\n");

    for (int p = 0; p < NUM_PROCESSES; p++) {
        printf("|   P%-2d   |  %2d    %2d    %2d   |  %2d    %2d    %2d   |  %2d    %2d    %2d   |\n",
               p,
               sys->allocation[p][0], sys->allocation[p][1], sys->allocation[p][2],
               sys->max_demand[p][0], sys->max_demand[p][1], sys->max_demand[p][2],
               sys->need[p][0],       sys->need[p][1],       sys->need[p][2]);
    }

    printf("+---------+-------------------+-------------------+-------------------+\n");
    printf("| Available:  A = %-2d, B = %-2d, C = %-2d                                    |\n",
           sys->available[0], sys->available[1], sys->available[2]);
    printf("+=====================================================================+\n\n");
}

/**
 * Safety Algorithm:
 * Evaluates whether the system is in a safe state and computes a valid execution sequence.
 * Time Complexity: O(NUM_RESOURCES * NUM_PROCESSES^2)
 *
 * @param sys Pointer to current BankerSystem state
 * @param safe_seq Output buffer to store safe sequence if one exists
 * @return true if state is safe, false otherwise
 */
bool check_system_safety(const BankerSystem *sys, int safe_seq[NUM_PROCESSES]) {
    int work[NUM_RESOURCES];
    bool finished[NUM_PROCESSES];

    /* Step 1: Initialize Work = Available and Finish[i] = false */
    for (int r = 0; r < NUM_RESOURCES; r++) {
        work[r] = sys->available[r];
    }
    for (int p = 0; p < NUM_PROCESSES; p++) {
        finished[p] = false;
    }

    int completed_count = 0;

    /* Step 2: Iteratively find an unfinished process whose need can be satisfied */
    while (completed_count < NUM_PROCESSES) {
        bool candidate_found = false;

        for (int p = 0; p < NUM_PROCESSES; p++) {
            if (!finished[p]) {
                bool can_satisfy = true;
                for (int r = 0; r < NUM_RESOURCES; r++) {
                    if (sys->need[p][r] > work[r]) {
                        can_satisfy = false;
                        break;
                    }
                }

                /* Step 3: If eligible, simulate execution and reclaim its resources */
                if (can_satisfy) {
                    for (int r = 0; r < NUM_RESOURCES; r++) {
                        work[r] += sys->allocation[p][r];
                    }
                    safe_seq[completed_count++] = p;
                    finished[p] = true;
                    candidate_found = true;
                }
            }
        }

        /* If no process can complete in this pass, the system is unsafe */
        if (!candidate_found) {
            return false;
        }
    }

    /* All processes were safely scheduled */
    return true;
}

/**
 * Resource-Request Algorithm:
 * Determines if an incoming request by process `pid` can be safely granted.
 *
 * @param sys Pointer to BankerSystem state
 * @param pid Process ID initiating the request (0 to 4)
 * @param req Array representing requested instances for [A, B, C]
 * @return true if request was granted, false if rejected/deferred
 */
bool process_resource_request(BankerSystem *sys, int pid, const int req[NUM_RESOURCES]) {
    printf(">>> Process P%d submits request: [ A=%d, B=%d, C=%d ]\n",
           pid, req[0], req[1], req[2]);

    /* Rule 1: Check if Request <= Need */
    for (int r = 0; r < NUM_RESOURCES; r++) {
        if (req[r] > sys->need[pid][r]) {
            printf("[-] ERROR: Process P%d exceeded its maximum declared claim for resource %c!\n",
                   pid, 'A' + r);
            return false;
        }
    }

    /* Rule 2: Check if Request <= Available */
    for (int r = 0; r < NUM_RESOURCES; r++) {
        if (req[r] > sys->available[r]) {
            printf("[-] DEFERRED: Insufficient resources available. Process P%d must wait.\n", pid);
            return false;
        }
    }

    /* Rule 3: Tentatively allocate resources (Hypothetical State Transition) */
    for (int r = 0; r < NUM_RESOURCES; r++) {
        sys->available[r]       -= req[r];
        sys->allocation[pid][r] += req[r];
        sys->need[pid][r]       -= req[r];
    }

    /* Rule 4: Run safety check on hypothetical state */
    int tentative_seq[NUM_PROCESSES];
    if (check_system_safety(sys, tentative_seq)) {
        printf("[+] SUCCESS: Request can be GRANTED safely to P%d.\n", pid);
        printf("    New Valid Safe Sequence: < ");
        for (int i = 0; i < NUM_PROCESSES; i++) {
            printf("P%d%s", tentative_seq[i], (i == NUM_PROCESSES - 1) ? " " : ", ");
        }
        printf(">\n");
        return true;
    } else {
        /* Rollback tentative modifications to preserve safety */
        printf("[-] REJECTED: Granting request would result in an UNSAFE state!\n");
        printf("    Rolling back tentative allocation for P%d...\n", pid);
        for (int r = 0; r < NUM_RESOURCES; r++) {
            sys->available[r]       += req[r];
            sys->allocation[pid][r] -= req[r];
            sys->need[pid][r]       += req[r];
        }
        return false;
    }
}

int main(void) {
    printf("===================================================================\n");
    printf("     BANKER'S ALGORITHM SIMULATION -- DEADLOCK AVOIDANCE (MA3105)   \n");
    printf("     Student: Udit | Roll Number: 2401MC07                         \n");
    printf("===================================================================\n");

    BankerSystem system_state;
    banker_init(&system_state);
    print_system_state(&system_state, "INITIAL SYSTEM STATE CONFIGURATION");

    /* Initial baseline safety check */
    int safe_seq[NUM_PROCESSES];
    if (check_system_safety(&system_state, safe_seq)) {
        printf("[*] Initial system state is confirmed SAFE.\n");
        printf("[*] Found Safe Execution Sequence: < ");
        for (int i = 0; i < NUM_PROCESSES; i++) {
            printf("P%d%s", safe_seq[i], (i == NUM_PROCESSES - 1) ? " " : ", ");
        }
        printf(">\n\n");
    } else {
        printf("[!] CRITICAL: Initial state is UNSAFE. Aborting simulation.\n");
        return 1;
    }

    /* Scenario 1: Safe Request from Process P1 */
    printf("-------------------------------------------------------------------\n");
    printf(" SCENARIO 1: Process P1 requests [ 1, 0, 2 ]\n");
    printf("-------------------------------------------------------------------\n");
    const int request_scenario_1[NUM_RESOURCES] = {1, 0, 2};
    process_resource_request(&system_state, 1, request_scenario_1);
    print_system_state(&system_state, "SYSTEM STATE AFTER SCENARIO 1 (P1 REQUEST)");

    /* Scenario 2: Unsafe Request from Process P0 */
    printf("-------------------------------------------------------------------\n");
    printf(" SCENARIO 2: Process P0 requests [ 0, 2, 0 ]\n");
    printf("-------------------------------------------------------------------\n");
    const int request_scenario_2[NUM_RESOURCES] = {0, 2, 0};
    process_resource_request(&system_state, 0, request_scenario_2);
    print_system_state(&system_state, "SYSTEM STATE AFTER SCENARIO 2 (P0 REQUEST REJECTED)");

    printf("===================================================================\n");
    printf(" Bankers Algorithm Simulation completed successfully.\n");
    printf("===================================================================\n");
    return 0;
}
