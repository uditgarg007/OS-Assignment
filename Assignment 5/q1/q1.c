/*
 * Question 1: Mutual Exclusion using Peterson's Algorithm
 * Operating Systems Lab Assignment 5
 * Roll Number: 2401MC07 | Name: Udit
 *
 * This program establishes mutual exclusion between two processes (parent=0, child=1)
 * purely using Peterson's Algorithm on a shared memory page allocated via the shm_get() syscall.
 * No OS locks/primitives are used for the critical section synchronization itself.
 */

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define NUM_ITERATIONS 10

// Peterson's shared state structure stored in the shared memory page
struct peterson_shared {
  int flag[2];         // flag[i] indicates process i's intention to enter CS
  int turn;            // turn indicates whose turn it is when both intend to enter
  int shared_counter;  // shared resource incremented inside the CS
};

// Remainder section busy-wait delay to encourage interleaving and race conditions
static void
burn_cycles(int count)
{
  for(volatile int i = 0; i < count; i++) {
    // delay loop
  }
}

int
main(int argc, char *argv[])
{
  // Allocate or map the shared memory page
  uint64 shm_addr = shm_get();
  if(shm_addr == (uint64)-1) {
    printf("q1: shm_get failed to map shared page\n");
    exit(1);
  }

  struct peterson_shared *shared = (struct peterson_shared *)shm_addr;

  // Initialize shared variables before fork (process 0 / parent initialization)
  shared->flag[0] = 0;
  shared->flag[1] = 0;
  shared->turn = 0;
  shared->shared_counter = 0;

  int pid = fork();
  if(pid < 0) {
    printf("q1: fork failed\n");
    exit(1);
  }

  int self_id;
  int peer_id;

  if(pid == 0) {
    // Child process (Process 1)
    self_id = 1;
    peer_id = 0;

    // Child maps the existing shared page into its own address space
    shm_addr = shm_get();
    if(shm_addr == (uint64)-1) {
      printf("q1: child failed to map shared memory\n");
      exit(1);
    }
    shared = (struct peterson_shared *)shm_addr;
  } else {
    // Parent process (Process 0)
    self_id = 0;
    peer_id = 1;
  }

  for(int iter = 0; iter < NUM_ITERATIONS; iter++) {
    // ----------------- ENTRY SECTION -----------------
    shared->flag[self_id] = 1;
    shared->turn = peer_id;

    // Memory barrier: guarantee store ordering across multiple RISC-V cores
    __sync_synchronize();

    // Busy-wait while peer process wants to enter and it's their turn
    while(shared->flag[peer_id] == 1 && shared->turn == peer_id) {
      // spin
    }

    // ---------------- CRITICAL SECTION ----------------
    shared->shared_counter++;
    printf("Process %d in CS, counter = %d\n", self_id, shared->shared_counter);

    // ----------------- EXIT SECTION ------------------
    __sync_synchronize();
    shared->flag[self_id] = 0;

    // --------------- REMAINDER SECTION ---------------
    burn_cycles(100000 + 40 * self_id);
  }

  // Parent waits for child process and verifies final counter value
  if(pid > 0) {
    wait(0);
    int expected = 2 * NUM_ITERATIONS;
    printf("Final shared_counter = %d (expected %d)\n", shared->shared_counter, expected);
  }

  exit(0);
}
