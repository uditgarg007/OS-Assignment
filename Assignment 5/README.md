# Operating Systems Lab Assignment 5: Process Synchronization in xv6-riscv

**Student Information**  
- **Name:** Udit  
- **Roll Number:** 2401MC07  
- **Course:** MA3105 / Operating Systems Lab  
- **Assignment:** Assignment 5 (Process Synchronization)  

---

## 1. Directory Structure

The assignment is organized with a clean, modular structure:

```
2401MC07_Udit_A5/
├── Makefile                # Build configuration registering q1, q2, q3, q4 in UPROGS
├── README.md               # Unified technical documentation (this file)
├── kernel/                 # Kernel source files (added and modified)
│   ├── defs.h              # Declarations for shminit, shm_get, shm_unmap, seminit, sem_*
│   ├── main.c              # Kernel entry point invoking shminit() and seminit()
│   ├── memlayout.h         # Memory layout defining SHMVA (TRAPFRAME - 2*PGSIZE)
│   ├── proc.c              # Process lifecycle hooks (shm_mapped initialization & shm_unmap)
│   ├── proc.h              # Extended struct proc with shm_mapped tracking flag
│   ├── sem.c               # Lightweight counting semaphore subsystem (sleep/wakeup)
│   ├── shm.c               # Shared physical page allocator and virtual mapping subsystem
│   ├── syscall.c           # System call dispatch table for shm_get, sem_init, sem_wait, sem_signal
│   ├── syscall.h           # System call numbers (SYS_shm_get, SYS_sem_init, SYS_sem_wait, SYS_sem_signal)
│   └── sysproc.c           # System call entry wrappers in kernel space
├── user/                   # User-level programs and headers
│   ├── q1.c                # Q1: Peterson's Algorithm mutual exclusion test
│   ├── q2.c                # Q2: Bounded buffer Producer-Consumer problem
│   ├── q3.c                # Q3: Readers-Writers problem with starvation-freedom
│   ├── q4.c                # Q4: Dining Philosophers with deadlock avoidance
│   ├── user.h              # User API definitions including new system call prototypes
│   └── usys.pl             # Perl script generating system call user-space assembly stubs
├── output_logs/            # Terminal output screenshots for verification
│   ├── q1.png              # Peterson's mutual exclusion execution screenshot
│   ├── q2.png              # Producer-Consumer buffer-full & buffer-empty blocking screenshot
│   ├── q3_1.png            # Readers-Writers concurrent readers & exclusive write screenshot (Part 1)
│   ├── q3_2.png            # Readers-Writers execution screenshot (Part 2)
│   ├── q4_1.png            # Dining Philosophers state transitions screenshot (Part 1)
│   └── q4_2.png            # Dining Philosophers completion & deadlock-freedom screenshot (Part 2)
├── q1/                     # Modular folder for Question 1 (source, standalone shm, output, readme)
├── q2/                     # Modular folder for Question 2 (source, standalone sem, output, readme)
├── q3/                     # Modular folder for Question 3 (source, output, readme)
└── q4/                     # Modular folder for Question 4 (source, output, readme)
```

---

## 2. Compilation & Execution Instructions

To compile and launch xv6-riscv in QEMU:

```bash
# Clean previous build artifacts
make clean

# Compile kernel and file system image, then launch QEMU
make qemu
```

Once inside the xv6 shell (`$`), run the programs for each question:

```bash
# Question 1: Peterson's Algorithm
$ q1

# Question 2: Producer-Consumer (default buffer size = 5, produces 20 items)
$ q2
# Or with custom buffer size:
$ q2 8

# Question 3: Readers-Writers Problem (3 readers, 2 writers, 5 rounds each)
$ q3

# Question 4: Dining Philosophers (5 philosophers, default 5 cycles each)
$ q4
# Or with custom cycle count:
$ q4 10
```

To exit QEMU: press `Ctrl-A` then `x`.

---

## 3. Detailed Design & Analysis

### Question 1: Mutual Exclusion using Peterson's Algorithm

#### 1. Problem & Challenge
xv6 enforces strict process address-space isolation. Upon `fork()`, xv6 copies the page table and user pages (`uvmcopy()`), giving the child a distinct private copy of the parent's memory. To implement Peterson's Algorithm, the parent and child must read and write to the exact same physical memory region without using OS synchronization primitives.

#### 2. Kernel Design: `shm_get()`
We implemented a single-page shared memory subsystem in `kernel/shm.c`:
- **Virtual Address (`SHMVA`)**: Positioned at `TRAPFRAME - 2 * PGSIZE` in `kernel/memlayout.h`. This address sits safely below the trampoline and trapframe pages, well outside ordinary heap (`sbrk`) expansion.
- **Physical Allocation**: The first call to `shm_get()` allocates a fresh 4KB physical page via `kalloc()`, zeroes it, and maps it into the calling process's page table at `SHMVA` with user read/write permissions (`PTE_R | PTE_W | PTE_U`).
- **Child Mapping**: Subsequent calls (by the child process) map the **same** physical page (`shm_phys_page`) into the child's page table at `SHMVA`.
- **Reference Counting & Deallocation**: A global reference counter (`shm_refcount`) tracks mapping instances. When a process terminates, `freeproc()` invokes `shm_unmap(p)`, which unmaps the virtual address without immediately freeing the page. The physical frame is returned to `kfree()` only when `shm_refcount` drops to 0.

#### 3. User Program Design: `user/q1.c`
The shared memory page hosts the textbook Peterson variables:
```c
struct peterson_shared {
  int flag[2];         // flag[i] = 1 indicates intent to enter CS
  int turn;            // whose turn it is when both intend to enter
  int shared_counter;  // target shared resource
};
```
- **Mutual Exclusion Protocol**:
  ```c
  // Entry Section
  shared->flag[self_id] = 1;
  shared->turn = peer_id;
  __sync_synchronize(); // Hardware memory barrier for multi-core RISC-V
  while(shared->flag[peer_id] == 1 && shared->turn == peer_id)
    ; // Busy-wait

  // Critical Section
  shared->shared_counter++;
  printf("Process %d in CS, counter = %d\n", self_id, shared->shared_counter);

  // Exit Section
  __sync_synchronize();
  shared->flag[self_id] = 0;
  ```
- **Memory Ordering**: RISC-V features a relaxed memory model. Because xv6 runs with multiple harts (CPUs), compiler and architectural reordering can violate sequential consistency. `__sync_synchronize()` issues full memory fence instructions (`fence iorw, iorw`), ensuring that flag and turn stores become visible globally in program order.
- **Verification**:
  - Each process executes 10 iterations.
  - Final value reaches exactly `2 * 10 = 20` without lost updates.
  - Critical section access is strictly non-interleaved.

---

### Question 2: Producer-Consumer Problem (Bounded Buffer)

#### 1. Synchronization Primitives: Kernel Counting Semaphores
Rather than busy-waiting, Question 2 requires genuine blocking synchronization so that the producer and consumer yield the CPU when waiting.
We implemented counting semaphores in `kernel/sem.c` using xv6's native sleep/wakeup mechanism:
- `sem_init(id, value)`: Initializes semaphore `id` with an initial integer capacity.
- `sem_wait(id)` ($P$ / down):
  - Acquires the semaphore spinlock.
  - If `value <= 0`, calls `sleep_prepare(s)` to register the sleep channel under the process lock, releases the semaphore spinlock, and calls `sleep()` to transition the process state to `SLEEPING`.
  - Upon waking, re-acquires the semaphore spinlock and re-evaluates `value` in a `while` loop (preventing spurious wakeups and lost wakeup races).
  - Decrements `value` and releases spinlock.
- `sem_signal(id)` ($V$ / up):
  - Acquires spinlock, increments `value`, releases spinlock, and calls `wakeup(s)` to mark waiting processes as `RUNNABLE`.

#### 2. Bounded Buffer Architecture: `user/q2.c`
The shared buffer resides in shared memory (`shm_get()`):
```c
struct bounded_buffer {
  int capacity;
  int head; // next write slot (in)
  int tail; // next read slot (out)
  int data[MAX_CAPACITY];
};
```
Three semaphores are configured:
1. `SEM_EMPTY` (id 0): initialized to `capacity` (available empty slots).
2. `SEM_FULL` (id 1): initialized to `0` (available filled slots).
3. `SEM_MUTEX` (id 2): initialized to `1` (binary semaphore guarding buffer pointers).

- **Standard Bounded-Buffer Invariant**:
  - Producer: `wait(SEM_EMPTY) -> wait(SEM_MUTEX) -> insert -> signal(SEM_MUTEX) -> signal(SEM_FULL)`
  - Consumer: `wait(SEM_FULL) -> wait(SEM_MUTEX) -> remove -> signal(SEM_MUTEX) -> signal(SEM_EMPTY)`

#### 3. Demonstration of Blocking Behavior
- **Buffer-Empty Blocking**: At startup, `SEM_FULL` is 0. The consumer starts and immediately blocks inside `sem_wait(SEM_FULL)` until the producer inserts the first item and signals `SEM_FULL`.
- **Buffer-Full Blocking**: The consumer is assigned a larger artificial delay loop (`600,000` cycles) while the producer runs faster (`200,000` cycles). The buffer quickly reaches maximum capacity (e.g. 5/5), causing the producer's `sem_wait(SEM_EMPTY)` to block until the consumer removes an item and frees a slot.
- All 20 items are produced and consumed in FIFO sequence with zero duplication or omission.

---

### Question 3: Readers-Writers Problem

#### 1. Problem Specification
Multiple reader processes and multiple writer processes concurrently access a shared variable `shared_data`.
- Multiple readers must be allowed to read concurrently.
- Writers require exclusive access (no other writer and no reader may operate simultaneously).
- Readers-priority must not cause writer starvation.

#### 2. Synchronization Logic: Three-Semaphore Solution
In `user/q3.c`, three semaphores coordinate access:
1. `SEM_MUTEX` (id 0): Binary semaphore protecting `read_count`.
2. `SEM_WRT` (id 1): Exclusive access lock for `shared_data`. Held by a writer for its entire critical section, or held collectively by the reader group (acquired by the 1st reader entering and released by the last reader exiting).
3. `SEM_QUEUE` (id 2): Turnstile / fairness gate preventing writer starvation.

#### 3. Why the `SEM_QUEUE` Turnstile Prevents Starvation
Under a naive readers-preference scheme, continuous arriving readers increment `read_count` before the previous readers exit, keeping `read_count > 0` indefinitely and holding `SEM_WRT` forever, completely starving waiting writers.

With the turnstile:
- Both readers and writers must pass through `sem_wait(SEM_QUEUE)`.
- When a writer arrives, it acquires `SEM_QUEUE` and waits for `SEM_WRT`.
- While the writer is queued, any new reader that arrives is blocked on `SEM_QUEUE`.
- New readers cannot increment `read_count` or prolong the group lock. As soon as the active readers finish and exit, the writer acquires `SEM_WRT`, executes, and releases `SEM_QUEUE`.
- This ensures fair interleaving while preserving concurrent read capability.

#### 4. Verification from Output Logs
- Output prints contain PID, round index, and active reader counts.
- Overlapping `ENTER` and `read` statements from different reader PIDs demonstrate concurrent reading.
- Writers output tight `ENTER (exclusive)` ... `EXIT (exclusive)` blocks with zero interleaved operations from other processes.

---

### Question 4: Dining Philosophers Problem (Deadlock Avoidance)

#### 1. Problem Formulation
5 philosophers sit around a circular table with 5 forks (chopsticks). Each philosopher alternates between:
$$\text{THINKING} \longrightarrow \text{HUNGRY} \longrightarrow \text{EATING} \longrightarrow \text{THINKING}$$
To eat, a philosopher requires both the left fork ($id$) and right fork ($(id+1) \pmod 5$).

#### 2. Deadlock Avoidance Strategy: Resource Ordering (Hierarchical Allocation)
If every philosopher picks up their left fork first, a situation can arise where all 5 grab their left fork simultaneously and wait indefinitely for their right fork. This creates a **circular wait** (one of Coffman's four necessary deadlock conditions):
$$P_0 \to F_1 \to P_1 \to F_2 \to P_2 \to F_3 \to P_3 \to F_4 \to P_4 \to F_0 \to P_0$$

To break circular wait, our solution enforces a strict global total ordering on fork acquisition:
- Each fork is assigned a discrete index $0, 1, 2, 3, 4$ as a binary semaphore.
- Every philosopher acquires $\min(\text{left}, \text{right})$ first, and $\max(\text{left}, \text{right})$ second:
  ```c
  int first_fork  = (left < right) ? left : right;
  int second_fork = (left < right) ? right : left;

  sem_wait(first_fork);
  sem_wait(second_fork);
  ```

#### 3. Mathematical Proof of Deadlock Freedom
Suppose a deadlock occurs. Then there exists a cycle of processes waiting on forks held by others:
- Each process in the cycle holds one fork and is waiting for another.
- Under our rule, no process can acquire a higher-indexed fork and then wait on a lower-indexed fork.
- Consequently, traversing the cycle requires fork indices to strictly increase monotonically:
  $$F_{i_1} < F_{i_2} < \dots < F_{i_k} < F_{i_1}$$
  This is a contradiction ($F_{i_1} < F_{i_1}$ is impossible).
- Therefore, a circular wait cannot exist, proving that deadlock is mathematically impossible.

In practice:
- Philosophers 0, 1, 2, 3 pick left fork first, then right fork.
- Philosopher 4 has `left = 4` and `right = 0`. Since $\min(4, 0) = 0$, Philosopher 4 picks **right fork first, then left fork**. This asymmetric behavior naturally emerges from the total ordering without arbitrary special-casing.

#### 4. Liveness & Concurrency
- Non-adjacent philosophers (e.g. Philosopher 0 and Philosopher 2) can eat simultaneously since they share no common forks.
- All 5 philosophers successfully complete all 5 cycles without permanent blocking or starvation.

---

## 4. Verification & Output Log Reference

All output logs and execution screenshots are stored in `output_logs/` and modular question directories:

| Question | Program | Output Screenshot | Validation Highlights |
| :--- | :--- | :--- | :--- |
| **Q1: Peterson's Algorithm** | `$ q1` | `output_logs/q1.png` | Counter reaches 20; strictly alternating/mutual exclusion prints. |
| **Q2: Producer-Consumer** | `$ q2` | `output_logs/q2.png` | Initial consumer empty block; buffer-full producer block; 20 items consumed. |
| **Q3: Readers-Writers** | `$ q3` | `output_logs/q3_1.png`, `q3_2.png` | Concurrent reads (active readers = 2, 3); exclusive writer access. |
| **Q4: Dining Philosophers** | `$ q4` | `output_logs/q4_1.png`, `q4_2.png` | Complete state transitions; all 5 philosophers complete 5 cycles deadlock-free. |
