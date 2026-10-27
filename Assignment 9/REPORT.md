# Assignment 9 — Results

This report summarizes the xv6-public changes and test results. The corresponding patches, modified kernel files, test programs, and run evidence are grouped under [`exercises/`](exercises).

## 1. Page-table inspection

Added `sys_vmprint` (system call 22) and `vmprint()` in `vm.c`. The function iterates over page-directory entries below `KERNBASE`, uses `walkpgdir()` to inspect pages, and prints virtual and physical addresses with the P/W/U flags. It reports valid user-page and page-table-page counts.

A valid user page is present and has `PTE_U`. The stack guard page at `0x1000` is present but has `U=0`, so it is shown and labeled as a guard page but excluded from the user-page count. The `pttest` program reports 2 valid user pages initially and 6 after `sbrk(4*4096)`: exactly four additional pages.

Run details: [exercise README](exercises/01-page-table-inspection/README.md), [serial capture](exercises/01-page-table-inspection/evidence/serial-console.txt), [screenshot](exercises/01-page-table-inspection/evidence/page-table-inspection.png).

## 2. Lazy page allocation

For positive `sbrk()` requests, `sys_sbrk` increases the process size without allocating physical pages; shrinking still frees memory. `copyuvm()` skips pages that have not yet been allocated. `trap.c` handles `T_PGFLT`: when the fault address is within the process size and is not mapped, it allocates and clears a page, maps it with `PTE_W|PTE_U`, increments `pgfaults`, and resumes. An invalid fault reports the process and address and kills only that process. A `getfaults()` system call exposes the fault count. `mappages()` is non-static so the trap handler can call it.

`faulttest` grows by 32 pages, touches five, and observes five handled faults. Re-touching a mapped page adds no fault. A child accessing beyond its size is killed while the parent survives.

Run details: [exercise README](exercises/02-lazy-page-allocation/README.md), [serial capture](exercises/02-lazy-page-allocation/evidence/serial-console.txt), [screenshot](exercises/02-lazy-page-allocation/evidence/lazy-allocation-test.png).

## 3. FIFO and LRU page replacement

`pagerepl.c` simulates FIFO and LRU, printing frame contents and whether each reference is a hit or fault. For the reference string `7 0 1 2 0 3 0 4 2 3 0 3 2 1 2 0 1 7 0 1` with three frames, FIFO reports 15 page faults and LRU reports 12.

| Frames | FIFO faults | FIFO hit ratio | LRU faults | LRU hit ratio |
|---:|---:|---:|---:|---:|
| 3 | 15 | 25% | 12 | 40% |
| 4 | 10 | 50% | 8 | 60% |
| 5 | 9 | 55% | 7 | 65% |

Run details: [exercise README](exercises/03-page-replacement/README.md), [serial capture](exercises/03-page-replacement/evidence/serial-console.txt), [FIFO/LRU trace](exercises/03-page-replacement/evidence/fifo-lru-trace.png), [frame comparison](exercises/03-page-replacement/evidence/frame-count-comparison.png).

## Build and run evidence

- Each patch was applied to a clean xv6-public checkout at `eeb7b41` and built with the stock Makefile and `-Werror`.
- QEMU was not available in the original build environment. The xv6 boot and filesystem images were run in the v86 x86 emulator; the xv6 UART console was captured, and the included screenshots render the corresponding run output.
- The Q3 algorithm was also cross-checked by compiling `pagerepl.c` as a Linux program, with the same 15/12 fault counts.
