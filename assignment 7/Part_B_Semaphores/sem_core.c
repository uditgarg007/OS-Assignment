#include "types.h"
#include "defs.h"
#include "param.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"
#include "sem_core.h"

/* Kernel semaphore pool */
struct semaphore sem_pool[MAX_SEM];
struct spinlock  sem_pool_lock;

/* One-time initialisation called from main() */
void
semaphore_init_all(void)
{
  int i;
  initlock(&sem_pool_lock, "sem_pool");
  for (i = 0; i < MAX_SEM; i++) {
    initlock(&sem_pool[i].lk, "sem");
    sem_pool[i].value  = 0;
    sem_pool[i].active = 0;
  }
}

/* syscall: sem_open(id, initial_value) */
int
sem_open(int id, int init_val)
{
  if (id < 0 || id >= MAX_SEM || init_val < 0)
    return -1;

  acquire(&sem_pool_lock);
  acquire(&sem_pool[id].lk);

  sem_pool[id].value  = init_val;
  sem_pool[id].active = 1;

  release(&sem_pool[id].lk);
  release(&sem_pool_lock);
  return 0;
}

/* syscall: sem_down(id)  – P / wait operation */
int
sem_down(int id)
{
  if (id < 0 || id >= MAX_SEM)
    return -1;

  acquire(&sem_pool[id].lk);
  if (!sem_pool[id].active) {
    release(&sem_pool[id].lk);
    return -1;
  }

  sem_pool[id].value--;
  if (sem_pool[id].value < 0)
    sleep(&sem_pool[id], &sem_pool[id].lk);   /* block */

  release(&sem_pool[id].lk);
  return 0;
}

/* syscall: sem_up(id)  – V / signal operation */
int
sem_up(int id)
{
  if (id < 0 || id >= MAX_SEM)
    return -1;

  acquire(&sem_pool[id].lk);
  if (!sem_pool[id].active) {
    release(&sem_pool[id].lk);
    return -1;
  }

  sem_pool[id].value++;
  if (sem_pool[id].value <= 0)
    wakeup_one(&sem_pool[id]);   /* unblock one waiter */

  release(&sem_pool[id].lk);
  return 0;
}