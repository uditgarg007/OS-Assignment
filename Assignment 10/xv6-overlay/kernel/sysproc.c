#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

// Assignment 10 Q1: expose allocator counters through a no-argument syscall.
int
sys_memstats(void)
{
  printmemstats();
  return 0;
}

// Assignment 10 Q2: translate a user-supplied virtual address.
int
sys_vtop(void)
{
  int va;
  if(argint(0, &va) < 0)
    return -1;
  return vtop((uint)va);
}

// Assignment 10 Q3: report the mapping for a user virtual address.
int
sys_pageinfo(void)
{
  int va;
  if(argint(0, &va) < 0)
    return -1;
  return pageinfo((uint)va);
}

// Assignment 10 Q3: clear the writable bit on a mapped user page.
int
sys_setreadonly(void)
{
  int va;
  if(argint(0, &va) < 0)
    return -1;
  return setreadonly((uint)va);
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
