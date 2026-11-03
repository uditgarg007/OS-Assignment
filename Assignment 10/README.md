# Assignment 10 — xv6 Memory Management and Paging

This submission implements the four questions in [`OS_Assignment_10.pdf`](OS_Assignment_10.pdf) on xv6-public commit `eeb7b41`.

## Files

- [`q1_memstats.c`](q1_memstats.c), [`q2_vtop.c`](q2_vtop.c), [`q3_pageprotection.c`](q3_pageprotection.c), and [`q4_pagereplacement.c`](q4_pagereplacement.c) — standalone main/test programs, one for each question.
- [`xv6-overlay/kernel/`](xv6-overlay/kernel/) — modified xv6 kernel files, syscall interface, and Makefile changes.
- [`xv6-overlay/user/`](xv6-overlay/user/) — runnable xv6 user programs (`memtest`, `vttest`, `pagetest`, `pagereplace`).
- [`evidence/`](evidence/) — serial console outputs and QEMU screenshots captured from fresh boots.
- [`REPORT.md`](REPORT.md) — implementation summary, measured results, and conclusions.

## Build and run

Start with xv6-public at the recorded base revision:

```sh
git clone https://github.com/mit-pdos/xv6-public.git xv6-public
cd xv6-public
git checkout eeb7b41
```

Copy the contents of this submission's `xv6-overlay/kernel/` into the xv6 source root and copy `xv6-overlay/user/*.c` into that root. The overlay's Makefile includes the four test programs in `UPROGS`. Build and boot:

```sh
make TOOLPREFIX= CFLAGS='-fno-pic -static -fno-builtin -fno-strict-aliasing -O2 -Wall -Werror -Wno-error=array-bounds -Wno-error=infinite-recursion -MD -ggdb -m32 -fno-omit-frame-pointer -fno-stack-protector -fno-pie -no-pie' xv6.img fs.img
make qemu-nox
```

At the xv6 shell, run:

```text
memtest
vttest
pagetest
pagereplace
```

The two `-Wno-error` flags allow stock xv6 warnings from this GCC version in `mp.c` and `sh.c`; `-Werror` remains enabled for other warnings.
