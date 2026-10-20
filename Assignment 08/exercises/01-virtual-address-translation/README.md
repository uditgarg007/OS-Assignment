# Question 1: Implementing Virtual-to-Physical Address Translation

## Overview
On the x86 architecture, virtual memory addresses are translated into physical memory addresses by hardware using a two-level paging scheme. A 32-bit virtual address is divided into three distinct components:
- **Bits [31:22]** (10 bits): Page Directory Index (`PDX`)
- **Bits [21:12]** (10 bits): Page Table Index (`PTX`)
- **Bits [11:0]**  (12 bits): Byte Offset within the 4 KB page frame

This assignment introduces the `int va2pa(int va)` system call, which allows user-space programs to directly query this translation while logging the underlying page table details from the kernel.

---

## System Call Specification

`int va2pa(int va);`

- **Input**: A 32-bit virtual address (`va`).
- **Execution Steps**:
  1. Traverses the calling process's page directory (`myproc()->pgdir`) via `walkpgdir()`.
  2. Validates the Page Table Entry (PTE). If the entry is missing, lacks the present bit (`PTE_P`), or lacks the user-accessible bit (`PTE_U`), the call aborts and returns `-1`.
  3. Isolates the physical base address of the frame using `PTE_ADDR(*pte)`.
  4. Calculates the final physical address by combining the frame base with the page offset: `pa = PTE_ADDR(*pte) | (va & 0xFFF)`.
  5. Outputs kernel-level diagnostic information—specifically `pde_idx`, `pte_idx`, `offset`, and the permission flags `[P, W, U]`—to the console via `cprintf`.
- **Return Value**: Returns the translated physical address (`pa`) as an integer on success, or `-1` if the address is unmapped or restricted.

---

## Directory Contents & Modified Files

| File | Description |
| :--- | :--- |
| `xv6-overlay/kernel/vm.c` | Contains the core `va2pa()` implementation and makes `walkpgdir()` globally accessible. |
| `xv6-overlay/kernel/sysproc.c` | Implements the `sys_va2pa()` wrapper to retrieve the virtual address argument from user space. |
| `xv6-overlay/kernel/defs.h` | Declares kernel function prototypes for `va2pa()` and `walkpgdir()`. |
| `xv6-overlay/kernel/syscall.h` | Assigns the system call number (`#define SYS_va2pa 22`). |
| `xv6-overlay/kernel/syscall.c` | Maps `sys_va2pa` into the kernel's system call dispatch table. |
| `xv6-overlay/kernel/user.h` | Declares the `int va2pa(int va);` prototype for user-level programs. |
| `xv6-overlay/kernel/usys.S` | Defines the assembly trap stub using `SYSCALL(va2pa)`. |
| `xv6-overlay/user/va2pa_test.c` | User-space test suite evaluating global, stack, heap, unmapped, and forked memory addresses. |
| `verification/test-output.txt` | Raw text log of the terminal output during testing. |
| `verification/terminal-capture.svg` | Vector graphic capture of the terminal execution. |
| `integration-guide.md` | xv6 integration and build instructions. |

---

## Experimental Verification (`va2pa_test.c`)

1. **Global, Stack, and Heap Memory**:
   - **Global Variable** (`0x2ff8`): Maps to physical frame `0xdfde000` (Physical Address: `0xdfdeff8`).
   - **Local Stack Variable** (`0x3fe8`): Maps to physical frame `0xdfdc000` (Physical Address: `0xdfdcfe8`).
   - **Heap Allocation** (`0x4010`): Maps to physical frame `0xdfdb000` (Physical Address: `0xdfdb010`).

2. **Invalid and Restricted Address Handling**:
   - `va2pa(0x7FFFFFFF)` returns `-1` because no page table entry is allocated for this unmapped user address.
   - `va2pa(0x80000000)` returns `-1` because the `PTE_U` flag is cleared, protecting kernel memory from user-space access.

3. **Address Space Isolation Across `fork()`**:
   - Both the parent and child processes reference the exact same virtual address (`0x2ff8`) for `global_var`.
   - However, the child's physical address (`0xdfd8ff8`) differs from the parent's physical address (`0xdfdeff8`).
   - This confirms that xv6's `copyuvm()` allocates separate physical memory frames for child processes during a `fork()`.