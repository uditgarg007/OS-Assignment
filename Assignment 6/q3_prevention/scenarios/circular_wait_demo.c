/**
 * ============================================================================
 * Operating Systems Lab -- Assignment 6
 * Question 3: Standalone Bad Demonstration -- Circular Wait Deadlock
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
        printf("  [-] BLOCKED: %s is currently held by Process %d -> CIRCULAR WAIT HANG!\n",
               l->label, l->holder);
    } else {
        l->locked = true;
        l->holder = proc_id;
        printf("  [+] SUCCESS: %s acquired by %s.\n", l->label, proc_name);
    }
}

int main(void) {
    printf("===================================================================\n");
    printf("   QUESTION 3: DEADLOCK VULNERABILITY RUN (INCONSISTENT LOCK ORDER)\n");
    printf("   Student: Udit | Roll Number: 2401MC07                           \n");
    printf("===================================================================\n\n");

    /* Out-of-order acquisition sequence */
    acquire_lock(&lock1, 1, "Process A");
    acquire_lock(&lock2, 2, "Process B");
    acquire_lock(&lock2, 1, "Process A");
    acquire_lock(&lock1, 2, "Process B");

    printf("\n[!] VERIFICATION RESULT: CIRCULAR WAIT DEADLOCK ENCOUNTERED!\n");
    printf("    System hangs as neither process can progress or release its lock.\n");
    return 0;
}
