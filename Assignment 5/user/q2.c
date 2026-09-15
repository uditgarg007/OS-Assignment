/*
 * Question 2: Producer-Consumer Problem (Bounded Buffer)
 * Operating Systems Lab Assignment 5
 * Roll Number: 2401MC07 | Name: Udit
 *
 * Implements the classic bounded-buffer producer-consumer problem using:
 * - Two counting semaphores: SEM_EMPTY (counts available free slots)
 *                           SEM_FULL  (counts filled items ready to consume)
 * - One binary semaphore:   SEM_MUTEX (guards access to the circular buffer)
 * - Shared memory allocated via shm_get()
 * - Real blocking semaphores implemented in kernel via sleep_prepare(), sleep(), and wakeup()
 */

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define TOTAL_ITEMS      20
#define DEFAULT_CAPACITY  5
#define MAX_CAPACITY    100

// Semaphore identifiers in the kernel semaphore table
#define SEM_EMPTY 0
#define SEM_FULL  1
#define SEM_MUTEX 2

// Circular buffer structure residing in shared memory
struct bounded_buffer {
  int capacity;
  int head; // write index (in)
  int tail; // read index (out)
  int data[MAX_CAPACITY];
};

// Delay function simulating work between operations
static void
simulate_work(int iterations)
{
  for(volatile int i = 0; i < iterations; i++) {
    // busy loop delay
  }
}

// Helper to inspect approximate count for diagnostic logging
static int
get_approx_occupancy(struct bounded_buffer *buf)
{
  int count = buf->head - buf->tail;
  if(count < 0)
    count += buf->capacity;
  return count;
}

int
main(int argc, char *argv[])
{
  int capacity = DEFAULT_CAPACITY;
  if(argc > 1) {
    capacity = atoi(argv[1]);
    if(capacity < 1 || capacity > MAX_CAPACITY) {
      printf("q2: invalid capacity %s. Must be between 1 and %d\n", argv[1], MAX_CAPACITY);
      exit(1);
    }
  }

  // Obtain shared memory page for the circular buffer
  uint64 shm_addr = shm_get();
  if(shm_addr == (uint64)-1) {
    printf("q2: shm_get failed\n");
    exit(1);
  }

  struct bounded_buffer *buf = (struct bounded_buffer *)shm_addr;
  buf->capacity = capacity;
  buf->head = 0;
  buf->tail = 0;

  // Initialize kernel semaphores before forking
  // SEM_EMPTY begins at buffer capacity (all slots free)
  // SEM_FULL begins at 0 (no items in buffer)
  // SEM_MUTEX begins at 1 (mutex available)
  sem_init(SEM_EMPTY, capacity);
  sem_init(SEM_FULL, 0);
  sem_init(SEM_MUTEX, 1);

  printf("prodcons: starting with bufsize=%d, %d items\n", capacity, TOTAL_ITEMS);

  int pid = fork();
  if(pid < 0) {
    printf("q2: fork failed\n");
    exit(1);
  }

  if(pid == 0) {
    // ----------------- CONSUMER PROCESS (CHILD) -----------------
    shm_addr = shm_get();
    if(shm_addr == (uint64)-1) {
      printf("q2: consumer shm_get failed\n");
      exit(1);
    }
    buf = (struct bounded_buffer *)shm_addr;

    for(int i = 0; i < TOTAL_ITEMS; i++) {
      // Wait for an available item (blocks initially since full=0)
      sem_wait(SEM_FULL);
      sem_wait(SEM_MUTEX);

      int item = buf->data[buf->tail];
      buf->tail = (buf->tail + 1) % buf->capacity;
      printf("Consumer: removed %d (buffer now ~%d/%d)\n",
             item, get_approx_occupancy(buf), buf->capacity);

      sem_signal(SEM_MUTEX);
      sem_signal(SEM_EMPTY);

      // Consumer is given a larger delay so that the buffer fills up,
      // demonstrating that the producer blocks when the buffer is full.
      simulate_work(600000);
    }

    printf("Consumer: done, consumed %d items\n", TOTAL_ITEMS);
    exit(0);

  } else {
    // ----------------- PRODUCER PROCESS (PARENT) ----------------
    for(int item = 1; item <= TOTAL_ITEMS; item++) {
      // Wait for an empty slot (blocks once buffer is full)
      sem_wait(SEM_EMPTY);
      sem_wait(SEM_MUTEX);

      buf->data[buf->head] = item;
      buf->head = (buf->head + 1) % buf->capacity;
      printf("Producer: inserted %d (buffer now ~%d/%d)\n",
             item, get_approx_occupancy(buf), buf->capacity);

      sem_signal(SEM_MUTEX);
      sem_signal(SEM_FULL);

      // Producer has smaller delay than consumer
      simulate_work(200000);
    }

    printf("Producer: done, produced %d items\n", TOTAL_ITEMS);

    // Wait for consumer child to terminate
    wait(0);
    printf("prodcons: both processes finished\n");
  }

  exit(0);
}
