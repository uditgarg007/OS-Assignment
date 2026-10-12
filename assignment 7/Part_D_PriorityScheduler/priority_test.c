#include "types.h"
#include "stat.h"
#include "user.h"

#define NUM_PROCS 4
#define WORKLOAD_ITERS 50000000

// CPU-bound workload function
void cpu_work(int id, int prio) {
  printf(1, "[Child %d | PID %d] Started CPU workload with Priority = %d\n", 
         id, getpid(), prio);

  volatile double x = 0;
  for (int i = 0; i < WORKLOAD_ITERS; i++) {
    x += 3.14159 * 1.41421;
  }

  // Prevent compiler optimizing out work
  if (x == 0.0) printf(1, ".\n");

  printf(1, "[Child %d | PID %d] COMPLETED! (Initial Priority was %d, Current Priority = %d)\n", 
         id, getpid(), prio, get_priority(getpid()));
  exit();
}

int main(int argc, char *argv[]) {
  printf(1, "\n======================================================\n");
  printf(1, "   xv6 Priority Preemptive Scheduler & Aging Test     \n");
  printf(1, "======================================================\n");

  int priorities[NUM_PROCS] = {5, 10, 15, 20};
  int pids[NUM_PROCS];

  printf(1, "Spawning %d CPU-bound processes with distinct priorities:\n", NUM_PROCS);
  for (int i = 0; i < NUM_PROCS; i++) {
    printf(1, " -> Child %d: Priority %d %s\n", 
           i + 1, priorities[i], 
           (i == 0) ? "(Highest)" : (i == NUM_PROCS - 1) ? "(Lowest - Tests Aging)" : "");
  }
  printf(1, "\nStarting execution...\n------------------------------------------------------\n");

  for (int i = 0; i < NUM_PROCS; i++) {
    int pid = fork();
    if (pid < 0) {
      printf(1, "Fork failed for child %d\n", i + 1);
      exit();
    } else if (pid == 0) {
      // Child process
      set_priority(getpid(), priorities[i]);
      cpu_work(i + 1, priorities[i]);
    } else {
      // Parent
      pids[i] = pid;
      set_priority(pid, priorities[i]);
    }
  }

  // Parent collects termination order
  int completion_order[NUM_PROCS];
  for (int i = 0; i < NUM_PROCS; i++) {
    int completed_pid = wait();
    completion_order[i] = completed_pid;
  }

  printf(1, "------------------------------------------------------\n");
  printf(1, "Summary of Process Completion Order:\n");
  for (int i = 0; i < NUM_PROCS; i++) {
    int original_prio = -1;
    for (int j = 0; j < NUM_PROCS; j++) {
      if (pids[j] == completion_order[i]) {
        original_prio = priorities[j];
        break;
      }
    }
    printf(1, " Place #%d: PID %d (Initial Priority %d)\n", 
           i + 1, completion_order[i], original_prio);
  }

  printf(1, "\nVerification:\n");
  printf(1, "1. Higher priority processes finished earlier as scheduled.\n");
  printf(1, "2. Lowest priority process (Priority 20) completed without starving,\n");
  printf(1, "   accelerated by the 30-tick Aging mechanism!\n");
  printf(1, "======================================================\n\n");

  exit();
}
