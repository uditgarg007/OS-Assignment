/**
 * ============================================================================
 * Operating Systems Lab -- Assignment 6
 * Question 2: Deadlock Detection via Resource Allocation Graph (RAG)
 *             and Wait-For Graph (WFG) Reduction with DFS Cycle Detection
 *
 * Student Name   : Udit
 * Roll Number    : 2401MC07
 * Course         : MA3105 / Operating Systems Lab
 * ============================================================================
 *
 * Theoretical Foundations:
 * A Resource Allocation Graph (RAG) is a directed bipartite graph G = (V, E)
 * where V = P U R (Processes P and Resources R).
 * For systems where every resource class contains exactly one single unit:
 * - A directed edge P_i -> R_k represents a resource request.
 * - A directed edge R_k -> P_j represents resource assignment/allocation.
 *
 * By transitive contraction of resource nodes, the RAG reduces directly to a
 * Wait-For Graph (WFG) where directed edges exist strictly between processes:
 * Edge (P_i -> P_j) exists if and only if P_i is requesting resource R_k,
 * and R_k is currently allocated to process P_j.
 *
 * Theorem (Silberschatz et al.):
 * In a resource allocation system with single-instance resource types,
 * a deadlock exists IF AND ONLY IF the corresponding Wait-For Graph contains
 * at least one directed cycle.
 *
 * We implement Tarjan's / CLRS three-color Depth-First Search (DFS) algorithm
 * to detect directed cycles in O(|V| + |E|) time complexity.
 * ============================================================================
 */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#define MAX_PROC 16
#define MAX_RES  16

/* DFS Node Exploration State (Three-Color Graph Coloring Scheme) */
typedef enum {
    COLOR_WHITE = 0,  /* Unvisited: Node has not yet been discovered */
    COLOR_GRAY  = 1,  /* Exploring: Node is on the active DFS recursion stack */
    COLOR_BLACK = 2   /* Processed: Node and all descendants completely visited */
} NodeColor;

/* Representation of the Resource Allocation and Wait-For Graphs */
typedef struct {
    int num_proc;
    int num_res;
    int alloc[MAX_PROC][MAX_RES]; /* alloc[p][r] = 1 if resource r is assigned to p */
    int req[MAX_PROC][MAX_RES];   /* req[p][r] = 1 if process p requests resource r */
    int wfg[MAX_PROC][MAX_PROC];  /* wfg[i][j] = 1 if P_i is waiting for P_j */
} RAGContext;

/* DFS Cycle Tracing State */
static NodeColor color[MAX_PROC];
static int predecessor[MAX_PROC];
static int cycle_entry_node = -1;
static int cycle_back_node  = -1;

/**
 * Reduce the bipartite Resource Allocation Graph into a directed Wait-For Graph.
 * Rule: For single-unit resources, if P_i requests R_k and P_j holds R_k (i != j),
 *       then insert directed dependency edge P_i -> P_j into WFG.
 */
void construct_wait_for_graph(RAGContext *ctx) {
    const int n = ctx->num_proc;
    const int m = ctx->num_res;

    /* Initialize adjacency matrix to 0 */
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            ctx->wfg[i][j] = 0;
        }
    }

    /* Edge derivation: P_i requests R_k while P_j holds R_k */
    for (int i = 0; i < n; i++) {
        for (int k = 0; k < m; k++) {
            if (ctx->req[i][k] > 0) {
                for (int j = 0; j < n; j++) {
                    if (i != j && ctx->alloc[j][k] > 0) {
                        ctx->wfg[i][j] = 1;
                    }
                }
            }
        }
    }
}

/**
 * Pretty-print the matrices and resulting Wait-For Graph.
 */
void display_graph_matrices(const RAGContext *ctx, const char *scenario_title) {
    printf("\n+=====================================================================+\n");
    printf("| SCENARIO: %-57s |\n", scenario_title);
    printf("+=====================================================================+\n");

    /* 1. Allocation Matrix */
    printf("\n[1] Allocation Matrix (Resources held by each Process):\n");
    printf("    Proc  |");
    for (int k = 0; k < ctx->num_res; k++) printf("  R%-2d", k);
    printf("\n    ------+");
    for (int k = 0; k < ctx->num_res; k++) printf("------");
    printf("\n");
    for (int i = 0; i < ctx->num_proc; i++) {
        printf("     P%-2d  |", i);
        for (int k = 0; k < ctx->num_res; k++) {
            printf("   %d ", ctx->alloc[i][k]);
        }
        printf("\n");
    }

    /* 2. Request Matrix */
    printf("\n[2] Request Matrix (Resources requested by each Process):\n");
    printf("    Proc  |");
    for (int k = 0; k < ctx->num_res; k++) printf("  R%-2d", k);
    printf("\n    ------+");
    for (int k = 0; k < ctx->num_res; k++) printf("------");
    printf("\n");
    for (int i = 0; i < ctx->num_proc; i++) {
        printf("     P%-2d  |", i);
        for (int k = 0; k < ctx->num_res; k++) {
            printf("   %d ", ctx->req[i][k]);
        }
        printf("\n");
    }

    /* 3. Wait-For Graph Adjacency Matrix */
    printf("\n[3] Wait-For Graph (WFG) Adjacency Matrix:\n");
    printf("    Wait\\Hold |");
    for (int j = 0; j < ctx->num_proc; j++) printf("  P%-2d", j);
    printf("\n    ----------+");
    for (int j = 0; j < ctx->num_proc; j++) printf("------");
    printf("\n");
    for (int i = 0; i < ctx->num_proc; i++) {
        printf("       P%-2d    |", i);
        for (int j = 0; j < ctx->num_proc; j++) {
            printf("   %d ", ctx->wfg[i][j]);
        }
        printf("\n");
    }

    /* 4. Explicit Dependency Edge Listing */
    printf("\n[4] Active Dependency Wait Edges:\n");
    bool has_edges = false;
    for (int i = 0; i < ctx->num_proc; i++) {
        for (int j = 0; j < ctx->num_proc; j++) {
            if (ctx->wfg[i][j]) {
                printf("    * Process P%d is WAITING for Process P%d\n", i, j);
                has_edges = true;
            }
        }
    }
    if (!has_edges) {
        printf("    (No wait dependencies found; graph is disconnected)\n");
    }
}

/**
 * Recursive DFS helper for cycle detection using 3-color scheme.
 * A back-edge pointing to a node currently colored GRAY indicates a directed cycle.
 */
static bool dfs_explore(const RAGContext *ctx, int u) {
    color[u] = COLOR_GRAY; /* Mark node as currently on call stack */

    for (int v = 0; v < ctx->num_proc; v++) {
        if (ctx->wfg[u][v]) {
            if (color[v] == COLOR_GRAY) {
                /* Back-edge found! We hit an ancestor on the recursion stack */
                cycle_entry_node = v;
                cycle_back_node  = u;
                return true;
            }
            if (color[v] == COLOR_WHITE) {
                predecessor[v] = u;
                if (dfs_explore(ctx, v)) {
                    return true;
                }
            }
        }
    }

    color[u] = COLOR_BLACK; /* Node fully explored */
    return false;
}

/**
 * Detect deadlocks by finding cycles across all components of the Wait-For Graph.
 */
void evaluate_deadlock(const RAGContext *ctx) {
    const int n = ctx->num_proc;
    for (int i = 0; i < n; i++) {
        color[i] = COLOR_WHITE;
        predecessor[i] = -1;
    }
    cycle_entry_node = -1;
    cycle_back_node  = -1;

    bool cycle_detected = false;
    for (int i = 0; i < n; i++) {
        if (color[i] == COLOR_WHITE) {
            if (dfs_explore(ctx, i)) {
                cycle_detected = true;
                break;
            }
        }
    }

    printf("\n---------------------------------------------------------------------\n");
    if (cycle_detected) {
        printf("[!] DIAGNOSIS: DEADLOCK DETECTED IN THE SYSTEM!\n");
        printf("    Circular wait condition confirmed in the Wait-For Graph.\n");
        printf("    Deadlocked Cycle Path: ");

        /* Reconstruct cycle path from cycle_back_node back to cycle_entry_node */
        int cycle_stack[MAX_PROC];
        int depth = 0;
        cycle_stack[depth++] = cycle_entry_node;

        for (int curr = cycle_back_node; curr != cycle_entry_node && curr != -1; curr = predecessor[curr]) {
            cycle_stack[depth++] = curr;
        }
        cycle_stack[depth++] = cycle_entry_node;

        for (int step = depth - 1; step >= 0; step--) {
            printf("P%d%s", cycle_stack[step], (step == 0) ? "" : " -> ");
        }
        printf("\n");
    } else {
        printf("[+] DIAGNOSIS: NO DEADLOCK DETECTED.\n");
        printf("    The Wait-For Graph is strictly ACYCLIC (Topological sort possible).\n");
        printf("    All processes can complete without entering circular wait.\n");
    }
    printf("---------------------------------------------------------------------\n");
}

int main(void) {
    printf("===================================================================\n");
    printf("   DEADLOCK DETECTION USING RESOURCE ALLOCATION GRAPHS (MA3105)    \n");
    printf("   Student: Udit | Roll Number: 2401MC07                           \n");
    printf("===================================================================\n");

    /* =======================================================================
     * Scenario 1: Acyclic System (No Deadlock)
     * 4 Processes (P0-P3), 3 Resources (R0-R2)
     * P0 holds R0, requests R1 (held by P1)
     * P1 holds R1, requests R2 (held by P2)
     * P2 holds R2, requests nothing
     * P3 holds nothing, requests R0 (held by P0)
     * ======================================================================= */
    RAGContext sc1 = {
        .num_proc = 4,
        .num_res  = 3,
        .alloc = {
            {1, 0, 0}, /* P0 holds R0 */
            {0, 1, 0}, /* P1 holds R1 */
            {0, 0, 1}, /* P2 holds R2 */
            {0, 0, 0}  /* P3 holds none */
        },
        .req = {
            {0, 1, 0}, /* P0 requests R1 */
            {0, 0, 1}, /* P1 requests R2 */
            {0, 0, 0}, /* P2 requests none */
            {1, 0, 0}  /* P3 requests R0 */
        }
    };
    construct_wait_for_graph(&sc1);
    display_graph_matrices(&sc1, "Scenario 1: Acyclic Wait-For Graph (Deadlock-Free)");
    evaluate_deadlock(&sc1);

    /* =======================================================================
     * Scenario 2: Cyclic System with Deadlock
     * 4 Processes (P0-P3), 4 Resources (R0-R3)
     * P0 holds R0, requests R1 (held by P1)
     * P1 holds R1, requests R2 (held by P2)
     * P2 holds R2, requests R0 (held by P0)  <-- Closes cycle P0->P1->P2->P0
     * P3 holds R3, requests R2 (held by P2)
     * ======================================================================= */
    RAGContext sc2 = {
        .num_proc = 4,
        .num_res  = 4,
        .alloc = {
            {1, 0, 0, 0}, /* P0 holds R0 */
            {0, 1, 0, 0}, /* P1 holds R1 */
            {0, 0, 1, 0}, /* P2 holds R2 */
            {0, 0, 0, 1}  /* P3 holds R3 */
        },
        .req = {
            {0, 1, 0, 0}, /* P0 requests R1 */
            {0, 0, 1, 0}, /* P1 requests R2 */
            {1, 0, 0, 0}, /* P2 requests R0 (Cycle creator) */
            {0, 0, 1, 0}  /* P3 requests R2 */
        }
    };
    construct_wait_for_graph(&sc2);
    display_graph_matrices(&sc2, "Scenario 2: Cyclic Wait-For Graph with Circular Wait");
    evaluate_deadlock(&sc2);

    printf("\n===================================================================\n");
    printf(" Deadlock Detection Simulation completed successfully.\n");
    printf("===================================================================\n");
    return 0;
}
