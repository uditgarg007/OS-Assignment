# Question 1: Mutual Exclusion using Peterson's Algorithm

**Roll Number:** 2401MC07 | **Name:** Udit

## Overview
Implements Peterson's Algorithm to achieve mutual exclusion between two processes (parent = 0, child = 1) sharing a critical section on xv6-riscv using only shared memory variables (`flag[2]` and `turn`). No OS-level synchronization primitives or locks are used for mutual exclusion.

## Key Files
- `q1.c`: User-level test program running 10 iterations per process.
- `q1_shm.c` (and `kernel/shm.c`): Kernel shared memory implementation providing the `shm_get()` system call.
- `q1_output.png`: Terminal output screenshot confirming execution and mutual exclusion.

## Execution
Inside the xv6 shell:
```bash
$ q1
```

## Verification Highlights
- Each process increments `shared_counter` inside its critical section.
- Final counter reaches exactly `20` (no lost updates).
- Memory barriers (`__sync_synchronize()`) ensure strict store ordering across multiple RISC-V cores.
