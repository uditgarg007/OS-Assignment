# OS Assignment 7 – xv6 Kernel Extensions

**Course**: Operating Systems Lab  
**Student**: Udit Garg  

---

## Overview

This assignment extends the xv6 teaching OS with four independent kernel features, each isolated in its own sub-directory.

| Directory | Feature |
|-----------|---------|
| `Part_A_KernelThreads/`   | `clone()` and `join()` system calls for user-level threads |
| `Part_B_Semaphores/`      | Kernel counting semaphores (`sem_open`, `sem_down`, `sem_up`) |
| `Part_C_DeadlockHandling/`| Deadlock detection (RAG reduction) + Banker's avoidance algorithm |
| `Part_D_PriorityScheduler/`| Priority-based scheduler with aging to prevent starvation |

---

## Part A – Kernel Threads (`clone` / `join`)

### Key Files
- `kernel_proc.c` – modified kernel process manager with `clone()` and `join()`
- `thread_demo.c` – user-space test demonstrating race conditions on a shared counter

### Design
- `clone(fcn, arg, stack)` allocates a new PCB, **shares the parent's `pgdir`**, and sets up the user stack using x86 cdecl conventions.
- `join()` scans for zombie thread children and reclaims their kernel stack. It deliberately **does not free `pgdir`** since it is owned by the parent.
- The `is_thread` flag in the PCB distinguishes threads from full processes in `wait()` and `join()`.

---

## Part B – Semaphores

### Key Files
- `sem_core.c` – kernel semaphore implementation
- `sem_core.h` – semaphore struct definition
- `producer_consumer.c` – producer–consumer demo using semaphores

### Design
- A fixed pool of `MAX_SEM` semaphores stored in kernel memory.
- `sem_down()` implements the **P (wait)** operation; blocks via `sleep()` when value < 0.
- `sem_up()` implements the **V (signal)** operation; wakes one waiter via `wakeup_one()`.

---

## Part C – Deadlock Handling

### Key Files
- `resource_manager.c` – deadlock detection + Banker's algorithm

### Design
- **Detection** (`deadlock_detect`): resource-allocation-graph reduction algorithm. Marks processes as "finished" if their request can be satisfied; remaining unfinished processes are deadlocked.
- **Safety** (`safety_check`): Banker's safety algorithm to find a safe sequence.
- **Avoidance** (`try_allocate`): tentatively grants a request, runs safety check, rolls back if unsafe.
- **Pipe simulation**: demonstrates circular wait between two processes sharing two pipes and verifies detection algorithm.

---

## Part D – Priority Scheduler with Aging

### Key Files
- `sched_proc.c` – priority-based scheduler replacing the default round-robin

### Design
- Each process has a `priority` field (1 = highest, 20 = lowest; default 10).
- `set_proc_priority(pid, prio)` / `get_proc_priority(pid)` are new system calls.
- The scheduler picks the RUNNABLE process with the **lowest numerical priority**. Ties are broken in **round-robin** order using a cursor.
- `apply_aging()` is called on every timer tick: a RUNNABLE process waiting ≥ 30 ticks gets its priority decremented by 1 (closer to 1 = higher urgency). This prevents starvation.

---

## Sample Outputs

Each sub-directory contains a `sample_output.txt` showing expected program output when run inside xv6 QEMU.