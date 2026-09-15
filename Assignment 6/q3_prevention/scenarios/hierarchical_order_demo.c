/**
 * ============================================================================
 * Operating Systems Lab -- Assignment 6
 * Question 3: Standalone Fixed Demonstration -- Strict Hierarchical Ordering
 *
 * Student Name   : Udit
 * Roll Number    : 2401MC07
 * Course         : MA3105 / Operating Systems Lab
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct {
    const char *label;
    bool locked;
    int holder;
} LockResource;

static LockResource lock1 = {"Lock1 (Printer)", false, -1};
static LockResource lock2 = {"Lock2 (Scanner)", false, -1};

void acquire_lock(LockResource *l, int proc_id, const char *proc_name) {
    printf("[%s] ATTEMPTING acquisition of %s...\n", proc_name, l->label);
    if (l->locked) {
        printf("  [-] BLOCKED: %s is currently held by Process %d -> WAITING\n",
               l->label, l->holder);
    } else {
        l->locked = true;
        l->holder = proc_id;
        printf("  [+] SUCCESS: %s acquired by %s.\n", l->label, proc_name);
    }
}

void release_lock(LockResource *l, const char *proc_name) {
    if (l->locked) {
        l->locked = false;
        l->holder = -1;
        printf("[%s] RELEASED: %s.\n", proc_name, l->label);
    }
}

int main(void) {
    printf("===================================================================\n");
    printf("   QUESTION 3: DEADLOCK PREVENTION RUN (STRICT HIERARCHICAL ORDER) \n");
    printf("   Student: Udit | Roll Number: 2401MC07                           \n");
    printf("===================================================================\n\n");

    /* Process A acquires Lock1 then Lock2 */
    acquire_lock(&lock1, 1, "Process A");
    acquire_lock(&lock1, 2, "Process B"); /* Blocked on Lock1 */

    acquire_lock(&lock2, 1, "Process A");
    printf("[Process A] In critical section. Completing job.\n");
    release_lock(&lock2, "Process A");
    release_lock(&lock1, "Process A");

    printf("\n--- Process A released resources; Process B proceeds ---\n\n");

    /* Process B acquires Lock1 then Lock2 */
    acquire_lock(&lock1, 2, "Process B");
    acquire_lock(&lock2, 2, "Process B");
    printf("[Process B] In critical section. Completing job.\n");
    release_lock(&lock2, "Process B");
    release_lock(&lock1, "Process B");

    printf("\n[+] VERIFICATION RESULT: WORKLOAD COMPLETED SAFELY WITHOUT DEADLOCK!\n");
    return 0;
}
