# xv6 Integration: Lazy Memory Allocation

This exercise is a source overlay for `xv6-public`, not a standalone project.
From the exercise folder:

1. Copy the contents of `xv6-overlay/kernel/` into the xv6 source root,
   replacing the matching files.
2. Copy `xv6-overlay/user/lazy_allocation_test.c` into the xv6 source root.
3. Add `_lazy_allocation_test\` to `UPROGS` in the xv6 Makefile.
4. Run `make clean && make qemu-nox`, then run `lazy_allocation_test` in the
   xv6 shell.
