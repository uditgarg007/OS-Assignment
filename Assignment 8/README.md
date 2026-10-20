# Operating Systems Assignment 8

This assignment contains four xv6 memory-management exercises. Each exercise
includes its explanation, kernel/user source overlay, xv6 integration guide,
and verification artifacts.

## Exercises

| Exercise | Topic | Guide |
| --- | --- | --- |
| 01 | Virtual-to-physical address translation | [README](exercises/01-virtual-address-translation/README.md) |
| 02 | Physical memory statistics | [README](exercises/02-physical-memory-statistics/README.md) |
| 03 | Page-table visualization | [README](exercises/03-page-table-visualization/README.md) |
| 04 | Lazy memory allocation | [README](exercises/04-lazy-memory-allocation/README.md) |

## Layout

```text
Assignment 8/
├── documentation/          # Assignment handout and consolidated report
└── exercises/
    ├── 01-virtual-address-translation/
    ├── 02-physical-memory-statistics/
    ├── 03-page-table-visualization/
    └── 04-lazy-memory-allocation/
```

Each exercise folder contains:

- `xv6-overlay/kernel/` — kernel source/header files to integrate into xv6.
- `xv6-overlay/user/` — user-space test program.
- `verification/` — captured test output and terminal screenshot. Captures keep
  the original xv6 command names used when they were recorded.
- `integration-guide.md` — xv6 build and run instructions; this is not a
  standalone Makefile.
- `README.md` — exercise overview and implementation details.

The source overlays are not a complete xv6 checkout. See the relevant
integration guide before building an exercise.
