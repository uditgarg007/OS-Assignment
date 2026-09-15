# Question 2: Producer-Consumer Problem (Bounded Buffer)

**Roll Number:** 2401MC07 | **Name:** Udit

## Overview
Implements the classic Bounded Buffer Producer-Consumer problem in xv6-riscv using:
- A circular buffer allocated on a shared memory page via `shm_get()`.
- Two counting semaphores (`SEM_EMPTY`, `SEM_FULL`) and one binary mutex semaphore (`SEM_MUTEX`).
- Real kernel blocking semaphores implemented in `kernel/sem.c` using xv6's `sleep_prepare()`, `sleep()`, and `wakeup()`.

## Key Files
- `q2.c`: User-level program implementing producer and consumer processes.
- `q2_sem.c` (and `kernel/sem.c`): Kernel counting semaphore system calls (`sem_init`, `sem_wait`, `sem_signal`).
- `q2_output.png`: Terminal output screenshot confirming blocking and item transfer.

## Execution
Inside the xv6 shell:
```bash
$ q2
# Or specify buffer capacity:
$ q2 8
```

## Verification Highlights
- Producer produces 20 integers (1 to 20); consumer consumes all 20 in order.
- Consumer blocks initially on empty buffer (`SEM_FULL = 0`).
- Producer blocks when buffer reaches capacity (`SEM_EMPTY = 0`).
- No items are dropped, duplicated, or consumed out of sequence.
