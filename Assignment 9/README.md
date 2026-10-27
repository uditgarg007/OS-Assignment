# xv6 Virtual Memory Exercises

This project contains three xv6-public exercises covering page-table inspection, lazy allocation, and page-replacement algorithms. Each exercise includes its xv6 patch, relevant modified kernel files, a user test program, and run evidence.

| Exercise | Topic | Test | Observed result |
|---|---|---|---|
| [01 — Page-table inspection](exercises/01-page-table-inspection/README.md) | Add `vmprint()` to display a process page table | `pttest` | 2 valid user pages at startup; 6 after allocating four pages |
| [02 — Lazy page allocation](exercises/02-lazy-page-allocation/README.md) | Allocate pages on demand when handling page faults | `faulttest` | 5 faults for 5 touched pages; invalid child access does not stop the parent |
| [03 — Page replacement](exercises/03-page-replacement/README.md) | Compare FIFO and LRU replacement | `pagerepl` | For 3 frames, FIFO has 15 faults and LRU has 12 |

See the [results report](REPORT.md) for implementation notes and detailed measurements.

## Project layout

```text
exercises/
  01-page-table-inspection/
    kernel/      relevant modified xv6 source files
    tests/       user-space test program
    patches/     patch for the xv6-public base revision
    evidence/    serial-console capture and screenshots
  02-lazy-page-allocation/   same organization
  03-page-replacement/       same organization
```

Each exercise directory has its own README linking to the patch, test, and run evidence.

## Reproduce a run

The patches target [mit-pdos/xv6-public](https://github.com/mit-pdos/xv6-public) at commit `eeb7b41`. Apply one patch to a fresh checkout for each exercise; the page-allocation changes are not intended to be stacked.

```sh
git clone https://github.com/mit-pdos/xv6-public xv6-public
cd xv6-public
git checkout eeb7b41
patch -p1 < "/path/to/Assignment 9/exercises/01-page-table-inspection/patches/xv6.patch"
make qemu-nox
```

Replace `/path/to/Assignment 9` with the absolute path to this project. At the xv6 prompt, run `pttest`, `faulttest`, or `pagerepl`, matching the patch you applied.

For the page-replacement reference-string demo, run:

```text
pagerepl 3 7,0,1,2,0,3,0,4,2,3,0,3,2,1,2,0,1,7,0,1
pagerepl table 7,0,1,2,0,3,0,4,2,3,0,3,2,1,2,0,1,7,0,1
```

The comma-separated reference string keeps the command within xv6's shell argument limit.

## Test evidence

The included [results report](REPORT.md) records the build and execution process. Each exercise guide links to its serial-console text capture and screenshots.
