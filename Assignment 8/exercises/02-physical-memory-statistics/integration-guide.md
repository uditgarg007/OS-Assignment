# xv6 Integration: Physical Memory Statistics

This exercise is a source overlay for `xv6-public`, not a standalone project.
From the exercise folder:

1. Copy the contents of `xv6-overlay/kernel/` into the xv6 source root,
   replacing the matching files. This includes the shared `meminfo.h` header
   required by the kernel and user test.
2. Copy `xv6-overlay/user/memory_stats_test.c` into the xv6 source root.
3. Add `_memory_stats_test\` to `UPROGS` in the xv6 Makefile.
4. Run `make clean && make qemu-nox`, then run `memory_stats_test` in the xv6
   shell.
