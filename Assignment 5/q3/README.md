# Question 3: Readers-Writers Problem

**Roll Number:** 2401MC07 | **Name:** Udit

## Overview
Implements the Readers-Writers problem with starvation prevention using a 3-semaphore architecture:
- `SEM_MUTEX`: Guards access to the `read_count` variable.
- `SEM_WRT`: Enforces exclusive access to `shared_data` for writers and the reader group.
- `SEM_QUEUE`: Turnstile semaphore ensuring arriving readers cannot starve waiting writers indefinitely.

## Key Files
- `q3.c`: User-level program spawning 3 reader processes and 2 writer processes across 5 rounds.
- `q3_output_1.png`, `q3_output_2.png`: Output screenshots showing concurrent reading and exclusive writing.

## Execution
Inside the xv6 shell:
```bash
$ q3
```

## Verification Highlights
- Multiple readers simultaneously read `shared_data` (indicated by active reader counts > 1).
- Writers execute inside strictly exclusive blocks (`ENTER (exclusive)` ... `EXIT (exclusive)`).
- Turnstile gate prevents writer starvation while preserving readers-preference concurrency.
