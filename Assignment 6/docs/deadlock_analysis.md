# Deliverable Analysis: Questions 3 & 4 (Deadlock Prevention & Concurrency Control)

**Student Name:** Udit  
**Roll Number:** 2401MC07  
**Course:** MA3105 / Operating Systems Lab  
**Assignment:** Assignment 6 (Deadlocks and Concurrency Control)  

---

## 1. Question 3: Deadlock Prevention via Havender's Resource Ordering Protocol

### 1.1 Theoretical Framework: The Four Coffman Conditions
As formalized by E. G. Coffman, M. J. Elphick, and A. Shoshani (1971), a system deadlock can arise if and only if the following four operational conditions hold simultaneously:

1. **Mutual Exclusion:** At least one resource is held in a non-shareable mode (exclusive access).
2. **Hold and Wait:** A process must currently hold at least one resource while concurrently waiting to acquire additional resources held by other processes.
3. **No Preemption:** Resources cannot be forcibly preempted from a process; they can only be released voluntarily by the holding process upon task completion.
4. **Circular Wait:** There must exist a closed cycle of processes $\{P_0, P_1, \dots, P_{n-1}, P_0\}$ such that $P_i$ is waiting for a resource held by $P_{(i+1) \bmod n}$.

### 1.2 Mechanism of Invalidation: Breaking Circular Wait
Deadlock *prevention* protocols constrain resource request models to eliminate at least one of the four necessary conditions. While Mutual Exclusion, Hold-and-Wait, and Preemption are often impractical or costly to break in hardware and I/O devices (e.g., printers, storage controllers), the **Circular Wait** condition can be strictly eliminated using **Resource Ordering** (Havender, 1968):

1. Let $R = \{R_1, R_2, \dots, R_m\}$ denote the universe of all distinct system resources.
2. Assign a one-to-one mapping function $F: R \to \mathbb{N}$ that establishes a strict, global total order across all resources:
   $$R_i \prec R_j \iff F(R_i) < F(R_j)$$
3. **Protocol Invariant:** A process holding a set of resources can only request a new resource $R_k$ if:
   $$F(R_k) > \max \{F(R_h) \mid R_h \text{ is currently held by the process}\}$$

### 1.3 Mathematical Proof of Deadlock-Freedom
Assume, for contradiction, that a deadlock involving circular wait exists among $k$ processes:
$$P_0 \to P_1 \to P_2 \to \dots \to P_{k-1} \to P_0$$
where process $P_i$ holds resource $R_{h_i}$ and is blocked waiting for resource $R_{w_i}$ held by $P_{(i+1) \bmod k}$.

- Because $P_i$ holds $R_{h_i}$ and requests $R_{w_i}$, by the Resource Ordering Protocol:
  $$F(R_{w_i}) > F(R_{h_i}) \quad \forall i \in \{0, 1, \dots, k-1\}$$
- Because $R_{w_i}$ is currently held by $P_{(i+1) \bmod k}$, we have:
  $$R_{w_i} = R_{h_{(i+1) \bmod k}}$$
- Substituting yields the chain of strict inequalities:
  $$F(R_{h_0}) < F(R_{h_1}) < F(R_{h_2}) < \dots < F(R_{h_{k-1}}) < F(R_{h_0})$$
- This implies:
  $$F(R_{h_0}) < F(R_{h_0})$$
  which is a mathematical contradiction (an integer cannot be strictly less than itself).

Therefore, a circular wait chain cannot form, rendering deadlocks **provably impossible**.

---

## 2. Question 4: Combined Synchronization & Deadlock Avoidance Strategy

### 2.1 System Architecture
Question 4 demonstrates an integrated resource management protocol for multi-instance device pools:
- **Pool 0 (Printer):** 2 instances, assigned Hierarchical Rank 0.
- **Pool 1 (Scanner):** 1 instance, assigned Hierarchical Rank 1.
- **Pool 2 (Disk):** 2 instances, assigned Hierarchical Rank 2.

Five concurrent workloads ($P_0$ through $P_4$) execute across repeated operational cycles, each requiring concurrent access to two distinct device pools.

### 2.2 Dual-Tier Avoidance Invariants
The architecture enforces two complementary layers of control:

1. **Ascending Hierarchy Enforcement:**
   - Resource classes must be acquired in ascending rank order:
     $$\text{Printer (Rank 0)} \prec \text{Scanner (Rank 1)} \prec \text{Disk (Rank 2)}$$
   - Every process requesting a bundle $\{R_a, R_b\}$ always requests $\min(F(R_a), F(R_b))$ first, followed by $\max(F(R_a), F(R_b))$.
   - This globally eliminates circular dependencies between different resource classes.

2. **Atomic Capacity & Availability Checking:**
   - Each resource pool maintains an atomic counter of currently available units.
   - A process is granted a resource instance only if `available_count > 0`.
   - If a pool is temporarily depleted (e.g., the single Scanner instance being held by another process), the requesting process safely waits without over-allocating system limits or holding lower-ranked processes hostage.

3. **Symmetric LIFO Deallocation:**
   - Resources are released in reverse order of acquisition:
     $$\text{Release}(R_{\text{higher}}) \implies \text{Release}(R_{\text{lower}})$$
   - This ensures bottleneck resources with smaller capacities (such as the single Scanner) are relinquished as soon as the critical section work concludes.

### 2.3 Verification & Starvation-Freedom
Over multiple cycles, all 5 processes systematically acquire their requisite pairs, execute their critical workload, and cleanly release both resources. The strict partial ordering combined with capacity bounds guarantees that every waiting process eventually unblocks once preceding critical sections conclude, achieving 100% throughput with zero deadlock occurrences.
