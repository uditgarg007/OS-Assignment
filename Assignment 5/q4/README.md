# Question 4: Dining Philosophers Problem (Deadlock Avoidance)

**Roll Number:** 2401MC07 | **Name:** Udit

## Overview
Implements the 5-philosopher Dining Philosophers problem with deadlock avoidance using Resource Ordering (Hierarchical Allocation).

## Deadlock Avoidance Strategy
- Forks are assigned indices $0..4$ and managed as kernel binary semaphores.
- Each philosopher requires forks $\text{left} = id$ and $\text{right} = (id + 1) \pmod 5$.
- To break the circular wait condition, each philosopher acquires $\min(\text{left}, \text{right})$ first, followed by $\max(\text{left}, \text{right})$.
- Because indices form a strict total order ($0 < 1 < 2 < 3 < 4$), no cycle of hold-and-wait dependencies can exist, mathematically eliminating deadlock.

## Key Files
- `q4.c`: User-level program spawning 5 philosopher processes executing 5 thinking-eating cycles.
- `q4_output_1.png`, `q4_output_2.png`: Output screenshots confirming cycle completion and deadlock freedom.

## Execution
Inside the xv6 shell:
```bash
$ q4
# Or specify cycle count:
$ q4 10
```

## Verification Highlights
- Philosophers correctly log state transitions: `THINKING -> HUNGRY -> EATING -> THINKING`.
- Non-adjacent philosophers eat simultaneously.
- All 5 philosophers complete all cycles without any deadlock.
