/*
 * Question 4: Dining Philosophers Problem (Deadlock Avoidance)
 * Operating Systems Lab Assignment 5
 * Roll Number: 2401MC07 | Name: Udit
 *
 * Implements 5 philosophers and 5 forks (represented by binary semaphores 0..4).
 * Deadlock Avoidance Strategy: RESOURCE ORDERING (Hierarchical Resource Allocation).
 *
 * Each philosopher requires two forks: left = id, right = (id + 1) % 5.
 * To break Coffman's circular wait condition, every philosopher always acquires
 * min(left, right) first, then max(left, right).
 * Because fork indices follow a strict total ordering (0 < 1 < 2 < 3 < 4),
 * a circular wait dependency graph cannot form, mathematically guaranteeing deadlock freedom.
 */

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define NUM_PHILOSOPHERS 5
#define DEFAULT_CYCLES   5

static void
burn_cycles(int iterations)
{
  for(volatile int i = 0; i < iterations; i++) {
    // delay loop
  }
}

static inline int
get_left_fork(int id)
{
  return id;
}

static inline int
get_right_fork(int id)
{
  return (id + 1) % NUM_PHILOSOPHERS;
}

static void
run_philosopher(int id, int cycles)
{
  int pid = getpid();
  int left = get_left_fork(id);
  int right = get_right_fork(id);

  // Resource ordering: systematically pick the lower-numbered fork first
  int first_fork  = (left < right) ? left : right;
  int second_fork = (left < right) ? right : left;

  for(int c = 0; c < cycles; c++) {
    // ----------------- THINKING -----------------
    printf("Philosopher %d (pid %d): THINKING\n", id, pid);
    burn_cycles(200000 + 15000 * id);

    // ------------------ HUNGRY ------------------
    printf("Philosopher %d (pid %d): THINKING -> HUNGRY\n", id, pid);
    sem_wait(first_fork);
    sem_wait(second_fork);

    // ------------------ EATING ------------------
    printf("Philosopher %d (pid %d): HUNGRY -> EATING (forks %d & %d, cycle %d/%d)\n",
           id, pid, left, right, c + 1, cycles);
    burn_cycles(250000);

    // Put down forks (in reverse order)
    sem_signal(second_fork);
    sem_signal(first_fork);

    // ------------- BACK TO THINKING -------------
    printf("Philosopher %d (pid %d): EATING -> THINKING\n", id, pid);
  }

  printf("Philosopher %d (pid %d): completed all %d cycles\n", id, pid, cycles);
}

int
main(int argc, char *argv[])
{
  int cycles = DEFAULT_CYCLES;
  if(argc > 1) {
    cycles = atoi(argv[1]);
    if(cycles < 1) {
      printf("dining: cycles must be >= 1\n");
      exit(1);
    }
  }

  // Initialize binary semaphore for each fork (1 = available)
  for(int i = 0; i < NUM_PHILOSOPHERS; i++) {
    sem_init(i, 1);
  }

  printf("dining: starting %d philosophers, %d cycles each\n", NUM_PHILOSOPHERS, cycles);

  for(int i = 0; i < NUM_PHILOSOPHERS; i++) {
    int pid = fork();
    if(pid < 0) {
      printf("dining: fork failed\n");
      exit(1);
    } else if(pid == 0) {
      run_philosopher(i, cycles);
      exit(0);
    }
  }

  // Parent waits for all 5 philosopher processes to conclude
  for(int i = 0; i < NUM_PHILOSOPHERS; i++) {
    wait(0);
  }

  printf("dining: all %d philosophers finished %d cycles each -- no deadlock occurred\n",
         NUM_PHILOSOPHERS, cycles);
  exit(0);
}
