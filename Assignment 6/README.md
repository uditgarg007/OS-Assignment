# Operating Systems Lab Assignment 6: Deadlock Avoidance, Detection, and Prevention

**Student Information**  
- **Name:** Udit  
- **Roll Number:** 2401MC07  
- **Course:** MA3105 / Operating Systems Lab  
- **Assignment:** Assignment 6 (Deadlock Management & Concurrency Control)  

---

## 1. Repository & Directory Structure

```text
Assignment 6/
├── README.md                         # Technical documentation & theoretical analysis (this document)
├── docs/
│   └── deadlock_analysis.md          # Theoretical foundations & mathematical proofs (Coffman & Havender)
├── screenshots/                      # Verification terminal execution screenshots
│   ├── q1_bankers.png                # Banker's algorithm safe sequence & request check
│   ├── q2_rag_wfg.png                # RAG to WFG reduction & cycle detection
│   ├── q3_hierarchy.png              # Resource ordering deadlock prevention
│   └── q4_concurrency.png            # Multi-resource synchronization controller
├── q1_bankers/                       # Question 1: Banker's Algorithm Simulation
│   ├── bankers_algorithm.c           # Safety algorithm & resource-request algorithm implementation
│   └── output.log                    # Verified execution output log
├── q2_detection/                     # Question 2: Deadlock Detection via RAG / WFG
│   ├── rag_deadlock_detector.c       # Graph reduction & 3-color DFS cycle detection
│   └── output.log                    # Verified execution output log
├── q3_prevention/                    # Question 3: Deadlock Prevention via Resource Ordering
│   ├── resource_hierarchy.c          # Unified driver comparing circular wait vs. strict ordering
│   ├── output.log                    # Verified execution output log
│   └── scenarios/
│       ├── circular_wait_demo.c      # Inconsistent lock order demonstration (deadlock)
│       └── hierarchical_order_demo.c # Strict hierarchical lock order demonstration (safe)
└── q4_concurrency/                   # Question 4: Combined Synchronization & Deadlock Avoidance
    ├── multi_resource_controller.c   # Multi-instance device pools with hierarchical acquisition
    └── output.log                    # Verified execution output log
```

---

## 2. Compilation & Execution Instructions

All programs are implemented in standard C (C99/C11 compatible) with POSIX/standard library primitives and can be compiled using GCC on Linux, macOS, or Windows (via MinGW / WSL).

```bash
# Question 1: Banker's Algorithm
gcc -Wall -Wextra q1_bankers/bankers_algorithm.c -o q1_bankers/bankers_algorithm
./q1_bankers/bankers_algorithm

# Question 2: Deadlock Detection
gcc -Wall -Wextra q2_detection/rag_deadlock_detector.c -o q2_detection/rag_deadlock_detector
./q2_detection/rag_deadlock_detector

# Question 3: Resource Hierarchy (Unified, Bad, and Fixed)
gcc -Wall -Wextra q3_prevention/resource_hierarchy.c -o q3_prevention/resource_hierarchy
./q3_prevention/resource_hierarchy

gcc -Wall -Wextra q3_prevention/scenarios/circular_wait_demo.c -o q3_prevention/scenarios/circular_wait_demo
./q3_prevention/scenarios/circular_wait_demo

gcc -Wall -Wextra q3_prevention/scenarios/hierarchical_order_demo.c -o q3_prevention/scenarios/hierarchical_order_demo
./q3_prevention/scenarios/hierarchical_order_demo

# Question 4: Combined Synchronization & Avoidance
gcc -Wall -Wextra q4_concurrency/multi_resource_controller.c -o q4_concurrency/multi_resource_controller
./q4_concurrency/multi_resource_controller
```

---

## 3. Detailed Algorithmic Design & Analysis

### Question 1: Banker's Algorithm for Deadlock Avoidance

#### 1. Theoretical Concept
Dijkstra's Banker's Algorithm guarantees that the operating system never transitions into an **unsafe state**. A state is safe if there exists an execution sequence $\langle P_{s_0}, P_{s_1}, \dots, P_{s_{n-1}} \rangle$ such that each process $P_{s_i}$ can finish its declared maximum workload using currently available resources plus the resources already held by preceding processes.

#### 2. Data Structures & Matrices
- **`Allocation[N][M]`**: Number of instances of resource class $R_j$ currently allocated to process $P_i$.
- **`Max[N][M]`**: Maximum claim of resource class $R_j$ declared by process $P_i$.
- **`Need[N][M]`**: Remaining claim, where $\text{Need}[i][j] = \text{Max}[i][j] - \text{Allocation}[i][j]$.
- **`Available[M]`**: Vector of unallocated instances per resource class.

#### 3. Algorithms Implemented
- **Safety Algorithm:** Operates with temporary vectors `Work = Available` and boolean array `Finish = [false]`. It iteratively scans for unfinished processes satisfying $\text{Need}[i] \le \text{Work}$. Upon completion, $\text{Work} \leftarrow \text{Work} + \text{Allocation}[i]$ and $P_i$ is appended to the safe sequence. Complexity: $\mathcal{O}(M \cdot N^2)$.
- **Resource-Request Algorithm:** When process $P_i$ requests vector $\vec{R}$:
  1. If $\vec{R} \le \text{Need}[i]$ and $\vec{R} \le \text{Available}$, proceed to step 2; otherwise reject or defer.
  2. Tentatively deduct from `Available`, add to `Allocation[i]`, and deduct from `Need[i]`.
  3. Run the Safety Algorithm. If safe, commit the allocation. If unsafe, execute an atomic rollback to protect system stability.

#### 4. Test Scenarios Evaluated
- **Baseline:** Safe state confirmed with safe sequence: $\langle P_1, P_3, P_4, P_0, P_2 \rangle$.
- **Scenario 1 (Safe Request):** $P_1$ requests $[1, 0, 2]$. Granted safely; new safe sequence: $\langle P_1, P_3, P_4, P_0, P_2 \rangle$.
- **Scenario 2 (Unsafe Request):** $P_0$ requests $[0, 2, 0]$. Denied due to impending unsafe state; rolled back safely.

---

### Question 2: Deadlock Detection via Resource Allocation Graph Reduction

#### 1. Theoretical Concept
For single-instance resource systems, deadlock existence is strictly equivalent to the presence of a directed cycle in the **Wait-For Graph (WFG)**.

#### 2. Reduction Algorithm
Given bipartite RAG matrices `Allocation` and `Request`:
$$\text{WFG}[i][j] = 1 \iff \exists k \text{ such that } \text{Request}[i][k] > 0 \land \text{Allocation}[j][k] > 0 \quad (i \ne j)$$

#### 3. Three-Color DFS Cycle Detection
Nodes are classified into three exploration states:
- `COLOR_WHITE` (0): Unvisited node.
- `COLOR_GRAY` (1): Currently active on the DFS recursion stack.
- `COLOR_BLACK` (2): Fully explored node and all its descendants.

A directed cycle is detected if and only if DFS encounters a back-edge $(u, v)$ where $v$ is `COLOR_GRAY`. Backtracking pointers `predecessor[]` allow precise reconstruction and printing of the deadlocked cycle path. Complexity: $\mathcal{O}(|V| + |E|)$.

#### 4. Test Scenarios Evaluated
- **Scenario 1 (4 processes, 3 resources):** Graph is acyclic. Result: No deadlock detected.
- **Scenario 2 (4 processes, 4 resources):** Circular dependency found: $P_0 \to P_1 \to P_2 \to P_0$. Result: Deadlock detected with exact process cycle printed.

---

### Question 3: Deadlock Prevention via Havender's Resource Ordering

#### 1. Theoretical Concept
Deadlock prevention invalidates Coffman's fourth necessary condition: **Circular Wait**. Havender (1968) proved that imposing a global strict total order on all resource locks $F: R \to \mathbb{N}$ and requiring processes to acquire locks strictly in increasing rank order eliminates the possibility of circular waiting chains.

#### 2. Implementation & Comparison
- **Inconsistent Order (`scenarios/circular_wait_demo.c`):**
  - Process A acquires Lock1, then requests Lock2.
  - Process B acquires Lock2, then requests Lock1.
  - Interleaving leads to circular wait ($P_A$ holds Lock1 waiting for Lock2; $P_B$ holds Lock2 waiting for Lock1), producing complete system hang.
- **Hierarchical Order (`scenarios/hierarchical_order_demo.c`):**
  - Global rule: $\text{Lock1 (Rank 1)} < \text{Lock2 (Rank 2)}$.
  - Both processes must request Lock1 before Lock2.
  - Contention is resolved at Lock1; the winner completes and releases both locks, allowing the second process to proceed smoothly without hanging.

---

### Question 4: Combined Synchronization & Deadlock Avoidance

#### 1. System Architecture
Manages multi-instance hardware resource pools:
- **Printer:** 2 instances (Hierarchical Rank 0)
- **Scanner:** 1 instance (Hierarchical Rank 1)
- **Disk:** 2 instances (Hierarchical Rank 2)

#### 2. Avoidance Invariants
1. **Hierarchical Request Order:** All 5 processes acquire their required pairs strictly in ascending order ($r_1 < r_2$), breaking circular wait across resource pools.
2. **Capacity Bounds Checking:** Atomic counters track currently available instances. Processes wait safely if a pool is temporarily exhausted without causing system instability.
3. **LIFO Release:** Higher-ranked resources are released prior to lower-ranked resources, promptly freeing constrained single-instance devices (Scanner).
4. **Result:** Across 3 operational workload cycles, all 5 processes execute and conclude without starvation, race conditions, or deadlocks.

---

## 4. Summary Table of Approaches

| Approach | Question | Target Coffman Condition | Primary Mechanism | Time Complexity |
| :--- | :---: | :--- | :--- | :---: |
| **Avoidance** | Q1 | System State Trajectory | Banker's Algorithm (Safety & Request check) | $\mathcal{O}(M \cdot N^2)$ |
| **Detection** | Q2 | Cycle Identification | RAG $\to$ WFG reduction + Three-Color DFS | $\mathcal{O}(\|V\| + \|E\|)$ |
| **Prevention** | Q3 | Circular Wait | Havender's Strict Total Resource Ordering | $\mathcal{O}(1)$ per lock |
| **Hybrid** | Q4 | Circular Wait + Capacity | Ascending Rank Acquisition + Atomic Pools | $\mathcal{O}(1)$ per pool |
