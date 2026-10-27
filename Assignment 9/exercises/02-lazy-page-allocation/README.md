# Lazy Page Allocation

Defers physical page allocation for positive `sbrk()` requests until a user process first accesses each page. A page-fault handler allocates and maps valid pages on demand, while invalid accesses terminate only the offending process.

## Files

- [`patches/xv6.patch`](patches/xv6.patch) — changes to apply to the xv6-public base revision.
- [`kernel/`](kernel) — relevant modified xv6 kernel and user-interface source files.
- [`tests/faulttest.c`](tests/faulttest.c) — checks fault counting, demand allocation, and invalid child access.
- [`evidence/serial-console.txt`](evidence/serial-console.txt) and [`evidence/lazy-allocation-test.png`](evidence/lazy-allocation-test.png) — captured test output.

Apply this exercise's patch to a clean xv6-public checkout at commit `eeb7b41`, build xv6, and run `faulttest` at the xv6 prompt. The test touches five pages and expects five handled faults.
