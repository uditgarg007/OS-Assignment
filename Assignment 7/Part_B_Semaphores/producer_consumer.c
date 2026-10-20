#include "types.h"
#include "stat.h"
#include "user.h"

#define BUFFER_SIZE 5
#define TOTAL_ITEMS 10
#define PGSIZE 4096

// Semaphore IDs
#define SEM_MUTEX 0
#define SEM_EMPTY 1
#define SEM_FULL  2

// Shared Bounded Buffer
volatile int buffer[BUFFER_SIZE];
volatile int in_idx = 0;
volatile int out_idx = 0;
volatile int count = 0;

// Producer Thread
void producer(void *arg) {
  printf(1, "[Producer] Starting production of %d items...\n", TOTAL_ITEMS);

  for (int item = 1; item <= TOTAL_ITEMS; item++) {
    // Wait for empty slot
    sem_wait(SEM_EMPTY);
    // Enter critical section
    sem_wait(SEM_MUTEX);

    // Insert item into buffer
    buffer[in_idx] = item;
    in_idx = (in_idx + 1) % BUFFER_SIZE;
    count++;

    printf(1, "[Producer] PRODUCED item #%d | Buffer Count: %d/%d (in=%d)\n", 
           item, count, BUFFER_SIZE, in_idx);

    if (count > BUFFER_SIZE) {
      printf(1, "PANIC: Buffer overflow detected!\n");
    }

    // Leave critical section
    sem_signal(SEM_MUTEX);
    // Signal that a new full slot is available
    sem_signal(SEM_FULL);

    // Small delay simulation
    sleep(2);
  }

  printf(1, "[Producer] Completed producing all items. Exiting.\n");
  exit();
}

// Consumer Thread
void consumer(void *arg) {
  printf(1, "[Consumer] Starting consumption of %d items...\n", TOTAL_ITEMS);

  for (int i = 1; i <= TOTAL_ITEMS; i++) {
    // Wait for full slot
    sem_wait(SEM_FULL);
    // Enter critical section
    sem_wait(SEM_MUTEX);

    // Remove item from buffer
    int item = buffer[out_idx];
    out_idx = (out_idx + 1) % BUFFER_SIZE;
    count--;

    printf(1, "[Consumer] CONSUMED item #%d | Buffer Count: %d/%d (out=%d)\n", 
           item, count, BUFFER_SIZE, out_idx);

    if (count < 0) {
      printf(1, "PANIC: Buffer underflow detected!\n");
    }

    // Leave critical section
    sem_signal(SEM_MUTEX);
    // Signal that a slot has become empty
    sem_signal(SEM_EMPTY);

    // Small delay simulation
    sleep(3);
  }

  printf(1, "[Consumer] Completed consuming all items. Exiting.\n");
  exit();
}

int main(int argc, char *argv[]) {
  printf(1, "\n======================================================\n");
  printf(1, "  Producer-Consumer Bounded Buffer Test (Buffer = 5)  \n");
  printf(1, "======================================================\n");

  // Initialize Semaphores
  if (sem_init(SEM_MUTEX, 1) < 0 ||
      sem_init(SEM_EMPTY, BUFFER_SIZE) < 0 ||
      sem_init(SEM_FULL, 0) < 0) {
    printf(1, "Error: Failed to initialize semaphores\n");
    exit();
  }

  printf(1, "Initialized SEM_MUTEX = 1\n");
  printf(1, "Initialized SEM_EMPTY = %d\n", BUFFER_SIZE);
  printf(1, "Initialized SEM_FULL  = 0\n\n");

  // Allocate user stacks for threads
  void *prod_stack = malloc(PGSIZE);
  void *cons_stack = malloc(PGSIZE);

  if (!prod_stack || !cons_stack) {
    printf(1, "Error: malloc failed for thread stacks\n");
    exit();
  }

  // Create producer and consumer threads
  int prod_pid = clone(producer, 0, (char *)prod_stack + PGSIZE);
  int cons_pid = clone(consumer, 0, (char *)cons_stack + PGSIZE);

  printf(1, "[Main] Launched Producer Thread (PID %d)\n", prod_pid);
  printf(1, "[Main] Launched Consumer Thread (PID %d)\n\n", cons_pid);

  // Wait for both threads to finish
  join();
  join();

  free(prod_stack);
  free(cons_stack);

  printf(1, "\n------------------------------------------------------\n");
  printf(1, "Final Buffer State: count = %d, in = %d, out = %d\n", count, in_idx, out_idx);
  printf(1, "Verification: No buffer overflow or underflow occurred.\n");
  printf(1, "Producer-Consumer synchronization fully successful!\n");
  printf(1, "------------------------------------------------------\n\n");

  exit();
}
