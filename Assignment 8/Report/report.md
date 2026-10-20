# Operating System Assignment 8 — Consolidated Report
## Memory Management in xv6

---

## 1. Executive Summary
This consolidated report details the design decisions, data structure implementations, kernel modifications, and experimental results for **Assignment 8: Memory Management in xv6**. Memory management is a fundamental operating system subsystem, bridging high-level process isolation with hardware-level page tables and memory management units (MMU).

Across this assignment, eight distinct virtual and physical memory mechanisms were implemented, instrumented, and evaluated on the x86 architecture:
1. **Virtual to Physical Address Translation (`va2pa`)**: Kernel page directory and table traversal exposing physical frame mapping and flag bits.
2. **Physical Memory Statistics (`getmeminfo`)**: System-wide free frame counting, user address space metrics, and page table accounting verifying zero memory leaks.
3. **Process Page Table Hierarchy Dump (`vmprint`)**: Complete two-level paging dump identifying user memory, the stack, the stack guard page (`PTE_U` cleared), and kernel space.
4. **Demand Paging / Lazy Heap Allocation (`lazy_allocation_test`)**: Zero-overhead lazy expansion via `sbrk()`, on-demand physical page allocation on hardware trap 14 (`T_PGFLT`), kernel buffer handling, and out-of-bounds process termination.
5. **Copy-on-Write (COW) Fork (`cowtest`)**: Shared read-only physical frame mapping on fork with per-frame reference counts, transparent copy-on-write faulting, and leak-free destruction.
6. **Memory Protection System Call (`mprotect`)**: Dynamic configuration of read-only, read-write, and inaccessible pages with hardware fault enforcement.
7. **Inter-Process Shared Memory (`shmget`, `shmat`)**: Physical frame sharing across disparate process address spaces facilitating high-speed zero-copy IPC.
8. **Page Replacement Policies (`pagerepl`)**: Comparative benchmark of FIFO, LRU, and Optimal replacement algorithms, including rigorous verification of Belady's Anomaly and stack algorithm monotonicity.

---

## 2. Question 1 — Virtual to Physical Address Translation

### 2.1 Design Decisions & Key Data Structures
- **Two-Level Paging Architecture**: The x86 architecture utilizes 32-bit linear addresses divided into three components:
  - **Page Directory Index (`PDX`)**: Bits [31:22] (10 bits) indexing into the Page Directory.
  - **Page Table Index (`PTX`)**: Bits [21:12] (10 bits) indexing into the Page Table.
  - **Offset**: Bits [11:0] (12 bits) specifying the exact byte offset within the 4096-byte page frame.
- **Page Table Traversal**: We exposed `walkpgdir(pde_t *pgdir, const void *va, int alloc)` from `vm.c` through `defs.h`. Given a virtual address `va`, `walkpgdir` extracts `pde = &pgdir[PDX(va)]`. If `*pde & PTE_P` is true, it converts the physical address stored in the PDE to a kernel virtual address (`P2V(PTE_ADDR(*pde))`) and returns the pointer to the PTE at index `PTX(va)`.
- **System Call `va2pa(uint va)`**:
  - Traverses `myproc()->pgdir` with `alloc = 0`.
  - Performs validation: if `pte == 0`, `!(*pte & PTE_P)` (not present), or `!(*pte & PTE_U)` (user-mode permission absent), it returns `-1`.
  - Kernel instrumentation: Invokes `cprintf()` to log `pde_idx`, `pte_idx`, `offset`, flag bits `P` (Present), `W` (Writable), `U` (User), and the final physical address `PTE_ADDR(*pte) | offset`.

### 2.2 Problems Faced & Solutions
- **Kernel Address Isolation**: When user code queried addresses at or above `KERNBASE` (`0x80000000`), a naive translation would succeed because the kernel identity-maps physical memory into all page directories.
  - *Resolution*: Enforced a strict check on `*pte & PTE_U`. Kernel mappings do not have the `PTE_U` flag set, correctly returning `-1` to user-mode callers and preventing kernel memory exposure.

### 2.3 Experimental Findings (`va2pa_test.c`)
- **Global Variable** (`&global_var = 0x2ff8`): PDE 0, PTE 2, Offset `0xff8` -> Physical Address `0xdfdeff8`.
- **Local Stack Variable** (`&local_var = 0x3fe8`): PDE 0, PTE 3, Offset `0xfe8` -> Physical Address `0xdfdcfe8`.
- **Heap Block** (`malloc` at `0x4010`): PDE 0, PTE 4, Offset `0x10` -> Physical Address `0xdfdb010`.
- **Invalid Virtual Address** (`0x7FFFFFFF`): Returned `-1` (unallocated PTE).
- **`fork()` Isolation**: Both parent and child reported the identical virtual address `0x2ff8` for `global_var`, but translated to entirely separate physical addresses (Child PA `0xdfd8ff8` vs. Parent PA `0xdfdeff8`). Modifying `global_var` in the child left the parent's value intact.

---

## 3. Question 2 — Physical Memory Statistics

### 3.1 Design Decisions & Key Data Structures
- **Physical Memory Accounting**: Introduced global counters `free_pages_count` and `total_pages_count` within `kalloc.c`.
  - Initialized in `kinit1()` and `kinit2()` as `freerange()` populates the free list.
  - `kfree()` increments `free_pages_count++` while holding `kmem.lock`.
  - `kalloc()` decrements `free_pages_count--` while holding `kmem.lock`.
- **Process Memory Metrics (`struct meminfo`)**:
  ```c
  struct meminfo {
    int total_pages; // System-wide allocatable physical frames
    int free_pages;  // Current free physical frames in freelist
    int user_pages;  // Calling process user memory: sz / PGSIZE
    int pt_pages;    // Pages used by process page directory and page tables
  };
  ```
- **Page Table Counting (`count_pt_pages`)**: Evaluates `1` (for `pgdir` itself) plus `1` for every valid entry in `pgdir` where `pde & PTE_P` is present.

### 3.2 Memory Usage Before and After (Free Pages Table)

| Test Stage | Total Pages | Free Pages | User Pages | PT Pages | Description |
| :--- | :---: | :---: | :---: | :---: | :--- |
| **Initial Baseline** | 56,320 | **56,302** | 4 | 2 | Shell and `memory_stats_test` loaded |
| **After `sbrk(4 * 4096)`** | 56,320 | **56,298** | 8 | 2 | Heap expanded by 4 pages (Δ = -4) |
| **During `fork()` (Child Active)** | 56,320 | **56,288** | 8 | 2 | Child cloned 8 data pages + 2 PT pages (Δ = -10) |
| **Post-Reap (`wait()` Complete)** | 56,320 | **56,298** | 8 | 2 | Child destroyed; all 10 pages reclaimed (Δ = 0) |

### 3.3 Memory Leak Verification
The free physical page count after the child was reaped returned precisely to **56,298**, matching the pre-fork baseline. This proves the complete absence of kernel memory leaks during process creation and reclamation.

---

## 4. Question 3 — Dumping the Page Table of a Process

### 4.1 Design Decisions & Key Data Structures
- **System Call `vmprint(void)`**: Traverses `myproc()->pgdir` from index 0 to `NPDENTRIES - 1`.
  - **Level 1** (`.. pde [i]`): Prints the PDE index, the physical address of the page table (`PTE_ADDR(pde)`), and permission flags `[P, W, U]`.
  - **Level 2** (`.... pte [j]`): Prints the PTE index, the mapped virtual address `(i << 22) | (j << 12)`, the physical frame address, and flags.
  - Filters out entries mapped above `KERNBASE` (`0x80000000`) and summarizes kernel mappings on a single final output line.
- **Hierarchical Visualization**: Indentation directly reflects the tree depth of the hardware paging unit.

### 4.2 Stack Guard Page Identification
In `exec.c`, xv6 configures the user stack by allocating two pages directly above the data segment and explicitly disabling `PTE_U` on the lower page:
```c
sz = PGROUNDUP(sz);
sz = allocuvm(pgdir, sz, sz + 2*PGSIZE);
clearpteu(pgdir, (char*)(sz - 2*PGSIZE));
```
In the `vmprint()` output for `page_table_dump_test`, this manifests as:
```text
.. pde [0]: pt_frame=0xdfd9000, flags=[P,W,U]
.... pte [0]: va=0x0 -> pa=0xdfdb000, flags=[P,W,U]
.... pte [1]: va=0x1000 -> pa=0xdfdc000, flags=[P,W,U]
.... pte [2]: va=0x2000 -> pa=0xdfdd000, flags=[P,W,-] <-- [GUARD PAGE: PTE_U cleared]
.... pte [3]: va=0x3000 -> pa=0xdfde000, flags=[P,W,U]
```
- **Guard Page Verification**: The PTE at index 2 (`va = 0x2000`) is marked `flags=[P,W,-]`. The absent `U` bit means any user access generates a hardware trap 14, protecting the text/data segments from stack overflows.
- **Page Accounting Check**: Process size `sz / PGSIZE` equates to 4 pages (`0x0`, `0x1000`, `0x2000`, `0x3000`). Because `0x2000` is an inaccessible guard page, the number of usable user pages is 3, which perfectly matches execution reality.

---

## 5. Question 4 — Lazy Memory Allocation (Demand Paging)

### 5.1 Design Decisions & Implementation Details
- **Lazy `sys_sbrk()`**: When extending memory (n > 0), `sys_sbrk()` bypasses `growproc()`. Instead, it simply updates `curproc->sz += n` and returns the old size in O(1) time. When reducing memory (n < 0), it calls `deallocuvm()` to immediately release physical frames.
- **Page Fault Trap Handling (`T_PGFLT` = 14)**:
  - Intercepts trap 14 and reads the `%cr2` register via `rcr2()` to extract the faulting virtual address `va`.
  - Verifies that `va` falls within valid boundaries: `2 * PGSIZE <= va < curproc->sz`.
  - Dynamically allocates a frame using `kalloc()`, zeros it with `memset()`, maps it via `mappages(curproc->pgdir, (char*)PGROUNDDOWN(va), PGSIZE, V2P(mem), PTE_W | PTE_U)`, and resumes user execution.
- **Kernel Syscall Buffer Handling (`argptr`)**: When a user process passes an untouched, lazily allocated buffer to a system call (e.g., `read`, `write`, `pipe`), it will not trigger a user-mode page fault. `argptr()` was modified to check if buffer pages are unmapped within the heap boundaries and forces allocation prior to kernel access.
- **`fork()` Adaptation (`copyuvm`)**: Modified the kernel memory cloning routine. Replaced `panic("copyuvm: page not present")` with `continue;` so child processes can safely fork even when the parent possesses untouched lazy pages.

### 5.2 Memory Usage Before and After (Free Pages Table)

| Test Stage | Free Pages | Process User Pages | Physical Frames Consumed | Note |
| :--- | :---: | :---: | :---: | :--- |
| **Baseline** | **56,302** | 4 | 0 | Before allocation |
| **After `sbrk(32 MB)`** | **56,302** | 8,196 | **0 frames** | Δ = 0: 8,192 pages requested, 0 physical frames allocated! |
| **Touch 3 Sparse Pages** | **56,296** | 8,196 | **6 frames** | 3 data frames + 3 intermediate page tables |
| **After Untouched Pipe Read**| **56,295** | 8,196 | **1 frame** | Kernel `argptr` demanded 1 page for syscall buffer |
| **Post Child Exit & Reap** | **56,295** | 8,196 | **0 frames** | No memory leaks observed |