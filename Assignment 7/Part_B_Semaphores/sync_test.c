#include "types.h"
#include "stat.h"
#include "user.h"

#define PGSIZE 4096
#define NUM_THREADS 3
#define INCREMENTS_PER_THREAD 100000
#define MUTEX_SEM 0

// Shared counter across all threads
volatile int shared_counter = 0;

// Synchronized thread worker function
void worker(void *arg) {
  int id = (int)(long)arg;
  printf(1, "[Thread %d] Started. Performing %d synchronized increments...\n", 
         id, INCREMENTS_PER_THREAD);

  for (int i = 0; i < INCREMENTS_PER_THREAD; i++) {
    // CRITICAL SECTION PROTECTED BY SEMAPHORE
    if (sem_wait(MUTEX_SEM) < 0) {
      printf(1, "Error: sem_wait failed in thread %d\n", id);
      exit();
    }

    shared_counter++; // Safe atomic critical section

    if (sem_signal(MUTEX_SEM) < 0) {
      printf(1, "Error: sem_signal failed in thread %d\n", id);
      exit();
    }
  }

  printf(1, "[Thread %d] Finished all increments safely. Exiting.\n", id);
  exit();
}

int main(int argc, char *argv[]) {
  printf(1, "\n======================================================\n");
  printf(1, "  xv6 Semaphore Synchronization Test (synctest)       \n");
  printf(1, "======================================================\n");

  // Initialize binary semaphore (mutex) with value 1
  if (sem_init(MUTEX_SEM, 1) < 0) {
    printf(1, "Error: sem_init failed for semaphore %d\n", MUTEX_SEM);
    exit();
  }
  printf(1, "Initialized Semaphore %d with value 1 (Binary Mutex)\n", MUTEX_SEM);
  printf(1, "Spawning %d threads, each incrementing shared_counter %d times...\n", 
         NUM_THREADS, INCREMENTS_PER_THREAD);
  printf(1, "Expected final counter: %d\n\n", NUM_THREADS * INCREMENTS_PER_THREAD);

  void *stacks[NUM_THREADS];
  int tids[NUM_THREADS];

  for (int i = 0; i < NUM_THREADS; i++) {
    stacks[i] = malloc(PGSIZE);
    if (!stacks[i]) {
      printf(1, "Error: Failed to allocate stack for thread %d\n", i + 1);
      exit();
    }

    void *stack_top = (char *)stacks[i] + PGSIZE;
    tids[i] = clone(worker, (void *)(long)(i + 1), stack_top);
    if (tids[i] < 0) {
      printf(1, "Error: clone failed for thread %d\n", i + 1);
      exit();
    }
    printf(1, "[Main] Created Thread %d with PID %d\n", i + 1, tids[i]);
  }

  // Join all threads
  for (int i = 0; i < NUM_THREADS; i++) {
    int joined_pid = join();
    printf(1, "[Main] Reaped Thread PID %d via join()\n", joined_pid);
    free(stacks[i]);
  }

  printf(1, "\n------------------------------------------------------\n");
  printf(1, "Final shared_counter Value: %d\n", shared_counter);
  printf(1, "Expected Value:             %d\n", NUM_THREADS * INCREMENTS_PER_THREAD);
  int lost_updates = (NUM_THREADS * INCREMENTS_PER_THREAD) - shared_counter;
  printf(1, "Lost Updates:               %d\n", lost_updates);
  printf(1, "------------------------------------------------------\n");

  if (shared_counter == (NUM_THREADS * INCREMENTS_PER_THREAD)) {
    printf(1, "SUCCESS: Semaphore mutex prevented all race conditions!\n");
    printf(1, "Result is exactly equal to theoretical expectation (300,000).\n\n");
  } else {
    printf(1, "FAILURE: Synchronization error observed! Lost %d updates.\n\n", lost_updates);
  }

  exit();
}
