/*
 * Question 3: Readers-Writers Problem
 * Operating Systems Lab Assignment 5
 * Roll Number: 2401MC07 | Name: Udit
 *
 * Implements the Readers-Writers problem with starvation prevention:
 * - Readers can read concurrently without blocking one another.
 * - Writers require strict mutual exclusion (no concurrent readers, no concurrent writers).
 * - A 'queue' turnstile semaphore prevents writers from being starved indefinitely
 *   by an ongoing stream of incoming readers.
 * - Spawns 3 readers and 2 writers via fork().
 */

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define NUM_READERS 3
#define NUM_WRITERS 2
#define NUM_ROUNDS  5

// Semaphore allocation
#define SEM_MUTEX 0   // Protects access to read_count
#define SEM_WRT   1   // Exclusive lock for shared_data (held by writer or first reader)
#define SEM_QUEUE 2   // Turnstile / fairness barrier to prevent writer starvation

// Shared state located on the shm page
struct rw_shared {
  int shared_data;
  int read_count;      // Active readers count; guarded by SEM_MUTEX
};

static void
burn_cycles(int iterations)
{
  for(volatile int i = 0; i < iterations; i++) {
    // delay loop
  }
}

static void
reader_routine(struct rw_shared *sh, int id)
{
  int pid = getpid();

  for(int r = 0; r < NUM_ROUNDS; r++) {
    // ---------------- ENTRY SECTION ----------------
    sem_wait(SEM_QUEUE);              // Pass through turnstile gate
      sem_wait(SEM_MUTEX);            // Protect read_count
        sh->read_count++;
        if(sh->read_count == 1) {
          sem_wait(SEM_WRT);          // First reader acquires exclusive write lock
        }
        printf("Reader %d (pid %d): ENTER  round %d (active readers = %d)\n",
               id, pid, r, sh->read_count);
      sem_signal(SEM_MUTEX);
    sem_signal(SEM_QUEUE);            // Free turnstile for next entrant

    // ------------- CRITICAL SECTION (READ) ----------
    burn_cycles(300000);              // Simulate reading duration
    printf("Reader %d (pid %d): read shared_data = %d\n", id, pid, sh->shared_data);

    // ----------------- EXIT SECTION -----------------
    sem_wait(SEM_MUTEX);
      sh->read_count--;
      printf("Reader %d (pid %d): EXIT   round %d (active readers = %d)\n",
             id, pid, r, sh->read_count);
      if(sh->read_count == 0) {
        sem_signal(SEM_WRT);         // Last reader releases write lock
      }
    sem_signal(SEM_MUTEX);

    // Remainder section: delay before next read request
    burn_cycles(150000 + 20000 * id);
  }
}

static void
writer_routine(struct rw_shared *sh, int id)
{
  int pid = getpid();

  for(int r = 0; r < NUM_ROUNDS; r++) {
    // ---------------- ENTRY SECTION ----------------
    sem_wait(SEM_QUEUE);              // Grab turnstile; blocks new readers
      sem_wait(SEM_WRT);              // Await exclusive write access

      // ------------- CRITICAL SECTION (WRITE) ---------
      printf("Writer %d (pid %d): ENTER  round %d (exclusive)\n", id, pid, r);
      burn_cycles(400000);            // Simulate writing duration
      sh->shared_data = sh->shared_data + 1;
      printf("Writer %d (pid %d): wrote shared_data = %d\n", id, pid, sh->shared_data);
      printf("Writer %d (pid %d): EXIT   round %d (exclusive)\n", id, pid, r);

      // ----------------- EXIT SECTION -----------------
      sem_signal(SEM_WRT);
    sem_signal(SEM_QUEUE);            // Release turnstile gate

    // Remainder section
    burn_cycles(200000 + 20000 * id);
  }
}

int
main(int argc, char *argv[])
{
  uint64 shm_addr = shm_get();
  if(shm_addr == (uint64)-1) {
    printf("q3: shm_get failed\n");
    exit(1);
  }

  struct rw_shared *sh = (struct rw_shared *)shm_addr;
  sh->shared_data = 0;
  sh->read_count = 0;

  // Initialize semaphores
  sem_init(SEM_MUTEX, 1);
  sem_init(SEM_WRT, 1);
  sem_init(SEM_QUEUE, 1);

  printf("readwrite: starting %d readers and %d writers, %d rounds each\n",
         NUM_READERS, NUM_WRITERS, NUM_ROUNDS);

  int total_workers = NUM_READERS + NUM_WRITERS;
  for(int i = 0; i < total_workers; i++) {
    int pid = fork();
    if(pid < 0) {
      printf("q3: fork failed\n");
      exit(1);
    } else if(pid == 0) {
      // Child process attaches to shared memory
      shm_addr = shm_get();
      if(shm_addr == (uint64)-1) {
        printf("q3: child shm_get failed\n");
        exit(1);
      }
      sh = (struct rw_shared *)shm_addr;

      if(i < NUM_READERS) {
        reader_routine(sh, i);
      } else {
        writer_routine(sh, i - NUM_READERS);
      }
      exit(0);
    }
  }

  // Parent waits for all reader and writer processes
  for(int i = 0; i < total_workers; i++) {
    wait(0);
  }

  int expected_val = NUM_WRITERS * NUM_ROUNDS;
  printf("readwrite: all done. final shared_data = %d (expected %d)\n",
         sh->shared_data, expected_val);

  exit(0);
}
