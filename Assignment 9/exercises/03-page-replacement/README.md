# Page Replacement: FIFO and LRU

Simulates FIFO and LRU page-replacement policies, showing frame contents and hit/fault status for each page reference. The program also compares fault counts and hit ratios across frame counts.

## Files

- [`patches/xv6.patch`](patches/xv6.patch) — changes to apply to the xv6-public base revision.
- [`kernel/Makefile`](kernel/Makefile) — xv6 user-program build-list change.
- [`tests/pagerepl.c`](tests/pagerepl.c) — replacement-policy simulator.
- [`evidence/serial-console.txt`](evidence/serial-console.txt), [`evidence/fifo-lru-trace.png`](evidence/fifo-lru-trace.png), and [`evidence/frame-count-comparison.png`](evidence/frame-count-comparison.png) — captured test output.

Apply the patch to a clean xv6-public checkout at commit `eeb7b41`, build xv6, and run `pagerepl` at the xv6 prompt. To reproduce the recorded 20-reference example, run:

```text
pagerepl 3 7,0,1,2,0,3,0,4,2,3,0,3,2,1,2,0,1,7,0,1
pagerepl table 7,0,1,2,0,3,0,4,2,3,0,3,2,1,2,0,1,7,0,1
```
