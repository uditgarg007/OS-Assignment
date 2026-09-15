/*
 * kernel/shm.c
 * Operating Systems Lab Assignment 5 - Process Synchronization
 * Roll Number: 2401MC07 | Name: Udit
 *
 * Kernel implementation of a shared-memory page mechanism for xv6-riscv.
 *
 * System call: uint64 sys_shm_get(void)
 * - On first invocation by any process, allocates a physical page via kalloc(),
 *   clears it, and maps it into the calling process's page table at SHMVA.
 * - On subsequent invocations by child processes, maps the identical physical
 *   page into the caller's page table at SHMVA.
 * - Returns SHMVA on success, or (uint64)-1 on allocation or mapping failure.
 * - Maintains a reference count (shm_refcount) so the physical page is safely freed
 *   via kfree() only when all processes that mapped it have exited (handled in shm_unmap()).
 */

#include "types.h"
#include "riscv.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

static struct spinlock shm_lock;
static char *shm_phys_page = 0;   // Pointer to single shared physical frame (0 = unallocated)
static int shm_refcount = 0;      // Number of active process mappings

// Initialize shared memory spinlock during kernel boot in main.c
void
shminit(void)
{
  initlock(&shm_lock, "shm_lock");
}

// System call body for shm_get()
uint64
shm_get(void)
{
  struct proc *p = myproc();

  // If this process already mapped the page, simply return the virtual address
  if(p->shm_mapped)
    return SHMVA;

  acquire(&shm_lock);
  if(shm_phys_page == 0) {
    // First caller: allocate fresh physical memory page
    release(&shm_lock);
    char *page = kalloc();
    if(page == 0)
      return (uint64)-1;
    memset(page, 0, PGSIZE);

    acquire(&shm_lock);
    if(shm_phys_page == 0) {
      shm_phys_page = page;
    } else {
      // Another core won the race to allocate; free ours and use existing
      release(&shm_lock);
      kfree(page);
      acquire(&shm_lock);
    }
  }
  char *pa = shm_phys_page;
  release(&shm_lock);

  // Map shared physical page into current process's page table at SHMVA with user read/write perms
  if(mappages(p->pagetable, SHMVA, PGSIZE, (uint64)pa, PTE_R | PTE_W | PTE_U) < 0)
    return (uint64)-1;

  acquire(&shm_lock);
  shm_refcount++;
  release(&shm_lock);

  p->shm_mapped = 1;
  return SHMVA;
}

// Called inside freeproc() when a process exits
void
shm_unmap(struct proc *p)
{
  if(!p->shm_mapped)
    return;

  // Unmap user virtual page without freeing the underlying physical page yet
  uvmunmap(p->pagetable, SHMVA, 1, 0);

  acquire(&shm_lock);
  shm_refcount--;
  if(shm_refcount <= 0 && shm_phys_page != 0) {
    kfree(shm_phys_page);
    shm_phys_page = 0;
    shm_refcount = 0;
  }
  release(&shm_lock);

  p->shm_mapped = 0;
}
