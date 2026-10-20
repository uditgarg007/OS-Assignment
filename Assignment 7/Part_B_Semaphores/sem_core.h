#ifndef _SEMAPHORE_H_
#define _SEMAPHORE_H_

#include "types.h"
#include "spinlock.h"

#define MAX_SEM 10

// Kernel counting semaphore structure
struct semaphore {
  int value;           // Current semaphore counter (< 0 indicates sleeping processes)
  int active;          // Allocation/initialization flag (1 = active, 0 = free)
  struct spinlock lk;  // Spinlock protecting this semaphore's state
};

// Prototypes for kernel semaphore operations
void sem_init_system(void);
int sem_init(int sem_id, int value);
int sem_wait(int sem_id);
int sem_signal(int sem_id);

#endif
