/**
 * ============================================================================
 * Operating Systems Lab -- Assignment 6
 * Question 3: Deadlock Prevention via Havender's Resource Ordering (Unified)
 *
 * Student Name   : Udit
 * Roll Number    : 2401MC07
 * Course         : MA3105 / Operating Systems Lab
 * ============================================================================
 *
 * Theoretical Foundations:
 * According to Coffman, Elphick, and Shoshani (1971), deadlock can only occur
 * if four conditions hold simultaneously:
 * 1. Mutual Exclusion
 * 2. Hold and Wait
 * 3. No Preemption
 * 4. Circular Wait
 *
 * Deadlock Prevention aims to design protocols that systematically invalidate
 * at least one of these conditions. Havender (1968) introduced the Resource
 * Hierarchy Protocol (Resource Ordering) to invalidate the CIRCULAR WAIT condition:
 * - Define a global one-to-one ranking function F: R -> N across all resources.
 * - Enforce the protocol: A process may request resource R_j if and only if
 *   F(R_j) > F(R_i) for all resources R_i currently held by that process.
 *
 * Mathematical Proof of Deadlock-Freedom:
 * Suppose a cycle of waiting processes exists: P0 -> P1 -> P2 -> ... -> Pk -> P0,
 * where P_i holds resource R_{h_i} and waits for R_{w_i} held by P_{(i+1) mod (k+1)}.
 * By the ordering rule, each process only requests higher-ranked resources:
 * F(R_{w_0}) > F(R_{h_0}), F(R_{w_1}) > F(R_{h_1}), ..., F(R_{w_k}) > F(R_{h_k}).
 * But since R_{w_i} is held by P_{i+1}, R_{w_i} = R_{h_{i+1}}.
 * Summing or chaining these inequalities implies:
 * F(R_{h_0}) < F(R_{h_1}) < ... < F(R_{h_k}) < F(R_{h_0}), which is an impossible
 * contradiction (a number cannot be strictly less than itself).
 * Hence, Circular Wait is mathematically impossible.
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/* Mutex abstraction with hierarchical rank */
typedef struct {
    const char *label;
    int rank;        /* Hierarchical priority rank */
    bool is_locked;
    int owner_id;    /* Process ID holding the lock (-1 if free) */
} HierarchicalLock;

/* Global locks representing shared devices */
static HierarchicalLock device_printer = {"Lock1 (Printer)", 1, false, -1};
static HierarchicalLock device_scanner = {"Lock2 (Scanner)", 2, false, -1};

static void reset_locks(void) {
    device_printer.is_locked = false;
    device_printer.owner_id  = -1;
    device_scanner.is_locked = false;
    device_scanner.owner_id  = -1;
}

static void try_lock(HierarchicalLock *l, int proc_id, const char *proc_name) {
    printf("[%s] Attempting to acquire %s [Rank %d]...\n", proc_name, l->label, l->rank);
    if (l->is_locked) {
        printf("  [-] BLOCKED: %s is currently HELD by Process P%d -> WAITING\n",
               l->label, l->owner_id);
    } else {
        l->is_locked = true;
        l->owner_id  = proc_id;
        printf("  [+] GRANTED: %s successfully locked by %s\n", l->label, proc_name);
    }
}

static void unlock(HierarchicalLock *l, const char *proc_name) {
    if (l->is_locked) {
        l->is_locked = false;
        l->owner_id  = -1;
        printf("[%s] RELEASED %s\n", proc_name, l->label);
    }
}

/**
 * Demonstration 1: Inconsistent Lock Acquisition Order (Deadlock Vulnerability)
 * Process A acquires Lock1 then requests Lock2
 * Process B acquires Lock2 then requests Lock1
 */
void run_inconsistent_order_demo(void) {
    reset_locks();
    printf("\n+=====================================================================+\n");
    printf("| DEMONSTRATION 1: INCONSISTENT LOCK ORDERING (CIRCULAR WAIT DEADLOCK)|\n");
    printf("+=====================================================================+\n");
    printf("Process A Strategy : Lock1 -> Lock2 (Rank 1 then Rank 2)\n");
    printf("Process B Strategy : Lock2 -> Lock1 (Rank 2 then Rank 1 - VIOLATION!)\n\n");

    /* Phase 1: Interleaved initial acquisitions */
    try_lock(&device_printer, 1, "Process A");
    try_lock(&device_scanner, 2, "Process B");

    printf("\n--- Interleaved execution pause (both hold 1 resource) ---\n\n");

    /* Phase 2: Crossed requests leading to circular wait */
    try_lock(&device_scanner, 1, "Process A");
    try_lock(&device_printer, 2, "Process B");

    printf("\n[!] DIAGNOSIS: DEADLOCK CONFIRMED!\n");
    printf("    Process A holds Lock1 and blocks waiting for Lock2.\n");
    printf("    Process B holds Lock2 and blocks waiting for Lock1.\n");
    printf("    Circular dependency: Process A <---> Process B\n");
}

/**
 * Demonstration 2: Strict Hierarchical Resource Ordering (Deadlock-Free)
 * Global Hierarchy: Rank(Lock1) < Rank(Lock2)
 * Both Process A and Process B must request Lock1 before Lock2.
 */
void run_hierarchical_order_demo(void) {
    reset_locks();
    printf("\n+=====================================================================+\n");
    printf("| DEMONSTRATION 2: STRICT HIERARCHICAL ORDERING (DEADLOCK PREVENTED) |\n");
    printf("+=====================================================================+\n");
    printf("Enforced Hierarchy Rule: Lock1 (Rank 1) < Lock2 (Rank 2)\n");
    printf("All processes must strictly acquire in ascending rank order.\n\n");

    /* Both contend for the lowest rank lock first */
    try_lock(&device_printer, 1, "Process A");
    try_lock(&device_printer, 2, "Process B"); /* Blocked: cannot proceed to Lock2 */

    /* Process A continues and acquires Lock2 */
    try_lock(&device_scanner, 1, "Process A");
    printf("[Process A] Both locks secured. Executing critical section workload...\n");
    unlock(&device_scanner, "Process A");
    unlock(&device_printer, "Process A");

    printf("\n--- Process A finished and released all locks; Process B wakes up ---\n\n");

    /* Process B can now safely acquire in prescribed order */
    try_lock(&device_printer, 2, "Process B");
    try_lock(&device_scanner, 2, "Process B");
    printf("[Process B] Both locks secured. Executing critical section workload...\n");
    unlock(&device_scanner, "Process B");
    unlock(&device_printer, "Process B");

    printf("\n[+] DIAGNOSIS: EXECUTION COMPLETED SUCCESSFULLY!\n");
    printf("    Strict total ordering eliminated Circular Wait.\n");
    printf("    All processes completed without blocking or hanging.\n");
}

int main(void) {
    printf("===================================================================\n");
    printf("   DEADLOCK PREVENTION VIA RESOURCE ORDERING PROTOCOL (MA3105)     \n");
    printf("   Student: Udit | Roll Number: 2401MC07                           \n");
    printf("===================================================================\n");

    run_inconsistent_order_demo();
    run_hierarchical_order_demo();

    printf("\n===================================================================\n");
    printf(" Resource Ordering Protocol demonstration completed successfully.  \n");
    printf("===================================================================\n");
    return 0;
}
