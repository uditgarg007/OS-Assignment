/*
 * kernel/sem.c
 * Operating Systems Lab Assignment 5 - Process Synchronization
 * Roll Number: 2401MC07 | Name: Udit
 *
 * Lightweight counting semaphore subsystem for xv6-riscv.
 * Implemented directly on top of xv6's native sleep/wakeup primitives:
 *   - sleep_prepare(chan): marks intent to sleep under process lock
 *   - sleep(): puts process into SLEEPING state until awakened
 *   - wakeup(chan): transitions sleeping processes back to RUNNABLE
 *
 * System calls provided:
 *   int sem_init(int id, int value)   - initializes semaphore 'id' to 'value'
 *   int sem_wait(int id)              - P() operation: decrements or blocks if <= 0
 *   int sem_signal(int id)            - V() operation: increments and wakes waiting process
 */

#include "types.h"
#include "riscv.h"
#include "param.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

#define MAX_SEMAPHORES 16

struct sem_descriptor {
  struct spinlock lock;  // Guards 'value' and serializes sleep/wakeup sequencing
  int value;
  int active;
};

static struct sem_descriptor sem_table[MAX_SEMAPHORES];

// Initialize all semaphore locks at kernel boot time (called in main.c)
void
seminit(void)
{
  for(int i = 0; i < MAX_SEMAPHORES; i++) {
    initlock(&sem_table[i].lock, "sem_lock");
    sem_table[i].value = 0;
    sem_table[i].active = 0;
  }
}

// Set initial value for semaphore id
int
sem_init(int id, int value)
{
  if(id < 0 || id >= MAX_SEMAPHORES)
    return -1;

  acquire(&sem_table[id].lock);
  sem_table[id].value = value;
  sem_table[id].active = 1;
  release(&sem_table[id].lock);
  return 0;
}

// Semaphore wait / P() / down() operation
// Blocks calling process if value <= 0 until signaled
int
sem_wait(int id)
{
  if(id < 0 || id >= MAX_SEMAPHORES)
    return -1;

  struct sem_descriptor *s = &sem_table[id];

  acquire(&s->lock);
  while(s->value <= 0) {
    // Register sleep channel before releasing semaphore lock to prevent lost wakeups
    sleep_prepare(s);
    release(&s->lock);
    sleep();
    acquire(&s->lock);
  }
  s->value--;
  release(&s->lock);
  return 0;
}

// Semaphore signal / V() / up() operation
// Increments value and awakens any process sleeping on this semaphore channel
int
sem_signal(int id)
{
  if(id < 0 || id >= MAX_SEMAPHORES)
    return -1;

  struct sem_descriptor *s = &sem_table[id];

  acquire(&s->lock);
  s->value++;
  release(&s->lock);

  wakeup(s);
  return 0;
}
