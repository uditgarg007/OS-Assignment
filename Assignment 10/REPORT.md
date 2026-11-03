# Assignment 10 — Results

The changes target xv6-public commit `eeb7b41`. Kernel modifications, standalone question programs, integrated xv6 user tests, and run evidence are included in this folder.

## Q1 — Physical memory statistics

`kalloc.c` tracks free pages and runtime allocation/free counts under the allocator lock. Boot-time free-list initialization is included in the available-page count but excluded from lifetime runtime counts. `memstats()` prints total, available, allocated, freed, and currently allocated pages. `memtest` allocates and touches three pages with `sbrk()`, then releases them.

In the recorded run, the physical allocator managed 57,000 pages. Available pages fell from 56,789 to 56,786 after the allocation and returned to 56,789 after release. Runtime allocated-minus-freed pages returned to its original count.

- [Q1 console output](evidence/q1-memtest.txt)
- [Q1 screenshot](evidence/q1-memtest.png)

## Q2 — Virtual-to-physical translation

`vtop()` uses `walkpgdir()` to validate a present user mapping and combines its physical frame address with the page offset. In the recorded run, VA `0x3000` mapped to PA `0xDFBC000`, and VA `0x3001` mapped to `0xDFBC001`. Unmapped VA `0x7FFFF000` correctly returned `-1`.

- [Q2 console output](evidence/q2-vttest.txt)
- [Q2 screenshot](evidence/q2-vttest.png)

## Q3 — Page information and read-only protection

`pageinfo()` reports the mapped frame and P/W/U flags. `setreadonly()` clears `PTE_W` and invalidates the corresponding TLB entry. In the test, W changed from 1 to 0; a write from a child process caused a page fault (trap 14), while the parent continued.

- [Q3 console output](evidence/q3-pagetest.txt)
- [Q3 screenshot](evidence/q3-pagetest.png)

## Q4 — Page replacement

The simulator traces FIFO, LRU, and Optimal for every reference in `7 0 1 2 0 3 0 4 2 3 0 3 2 1 2 0 1 7 0 1`.

| Frames | Algorithm | Faults | Hits | Hit ratio | Fault ratio |
|---:|---|---:|---:|---:|---:|
| 3 | FIFO | 15 | 5 | 25.00% | 75.00% |
| 3 | LRU | 12 | 8 | 40.00% | 60.00% |
| 3 | Optimal | 9 | 11 | 55.00% | 45.00% |
| 4 | FIFO | 10 | 10 | 50.00% | 50.00% |
| 4 | LRU | 8 | 12 | 60.00% | 40.00% |
| 4 | Optimal | 8 | 12 | 60.00% | 40.00% |
| 5 | FIFO | 9 | 11 | 55.00% | 45.00% |
| 5 | LRU | 7 | 13 | 65.00% | 35.00% |
| 5 | Optimal | 7 | 13 | 65.00% | 35.00% |

Optimal yields the fewest faults with three frames; it ties with LRU for four and five frames on this reference string.

- [Q4 full console trace and comparison table](evidence/q4-pagereplace.txt)
- [Q4 screenshot](evidence/q4-pagereplace.png)

## Validation

The xv6 kernel, filesystem, and all four test binaries built successfully; each test was run from a fresh QEMU boot. The build used `-Werror` while exempting two warning classes emitted by unchanged stock xv6 code with the current GCC.
