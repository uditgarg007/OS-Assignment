# Question 4: Lazy Memory Allocation (Demand Paging)

## Overview
By default, xv6 strictly utilizes eager allocation, assigning physical memory frames immediately whenever a process calls `sbrk(n)`. For modern operating systems, this approach is highly inefficient, as it wastes precious physical memory frames on sparse data structures, over-provisioned heaps, or massive memory-mapped buffers.

This assignment implements **Demand Paging (Lazy Memory Allocation)**, which defers physical memory assignment until it is actively required. The user-space test is `lazy_allocation_test.c`:
1. **Size Update**: `sys_sbrk(n)` only increments the process's tracked virtual size (`myproc()->sz += n`) without allocating any actual physical page frames.
2. **Page Fault Generation**: When a user process attempts to access an untouched heap page, the x86 MMU encounters an unmapped virtual address and triggers a hardware trap 14 (`T_PGFLT`).
3. **Kernel Resolution**: The kernel intercepts this `T_PGFLT` within `trap.c`, reads the faulting virtual address from the `%cr2` register, dynamically allocates a zero-filled 4 KB frame using `kalloc()`, and maps it into the page table via `mappages()` before seamlessly resuming the user process.

---

## Edge Cases Handled

1. **Out-of-Bounds & Negative Offsets**:
   - Virtual addresses that exceed the process size (`>= sz`) or dip below the user stack boundary (`< 2 * PGSIZE`) are flagged as invalid. The kernel terminates the offending process with an informative segmentation fault message rather than crashing the system.
2. **Kernel Dereference of Untouched Buffers (`argptr`)**:
   - If a user process passes an untouched virtual buffer to system calls like `read()`, `write()`, or `pipe()`, `argptr()` intervenes to lazily fault in the required physical frames *before* the kernel attempts to access them.
3. **`copyuvm()` in `fork()`**:
   - The standard xv6 implementation triggers a `panic("copyuvm: page not present")` upon encountering a missing page entry. The modified `copyuvm()` safely uses `continue;` to skip unallocated PTEs, ensuring child processes inherit the same lazy memory footprint without crashing.
4. **Physical Memory Exhaustion**:
   - In the event that `kalloc()` returns `0` (indicating the system is out of physical frames), the kernel gracefully terminates the faulting process rather than causing a system-wide panic.

---