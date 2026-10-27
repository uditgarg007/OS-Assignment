# Page-Table Inspection

Adds a `vmprint()` system call that prints a process's page-table mappings and counts present user pages.

## Files

- [`patches/xv6.patch`](patches/xv6.patch) — changes to apply to the xv6-public base revision.
- [`kernel/`](kernel) — relevant modified xv6 kernel and user-interface source files.
- [`tests/pttest.c`](tests/pttest.c) — test for the initial mapping count and growth after `sbrk()`.
- [`evidence/serial-console.txt`](evidence/serial-console.txt) and [`evidence/page-table-inspection.png`](evidence/page-table-inspection.png) — captured test output.

Apply this exercise's patch to a clean xv6-public checkout at commit `eeb7b41`, build xv6, and run `pttest` at the xv6 prompt. The test expects 2 valid user pages initially and 6 after adding four pages.
