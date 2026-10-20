# xv6 Integration: Virtual-to-Physical Translation

This exercise is a source overlay for `xv6-public`, not a standalone project.
From the exercise folder:

1. Copy the contents of `xv6-overlay/kernel/` into the xv6 source root,
   replacing the matching files.
2. Copy `xv6-overlay/user/va2pa_test.c` into the xv6 source root.
3. Add `_va2pa_test\` to `UPROGS` in the xv6 Makefile.
4. Run `make clean && make qemu-nox`, then run `va2pa_test` in the xv6 shell.
