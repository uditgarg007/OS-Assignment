#include "types.h"
#include "stat.h"
#include "user.h"

#define PROC_MAX 10
#define RES_MAX  10

/* Utility: dump allocation/request/available matrices */
void
show_state(int np, int nr,
           int alloc[PROC_MAX][RES_MAX],
           int req[PROC_MAX][RES_MAX],
           int avail[RES_MAX],
           const char *req_tag)
{
  int i, j;
  printf(1, "\nPID  |  Allocated      |  %-12s   |  Available\n", req_tag);
  printf(1, "-----+-----------------+-----------------+-----------------\n");
  for (i = 0; i < np; i++) {
    printf(1, " P%-2d |  ", i);
    for (j = 0; j < nr; j++) printf(1, "%d ", alloc[i][j]);
    printf(1, "       |  ");
    for (j = 0; j < nr; j++) printf(1, "%d ", req[i][j]);
    printf(1, "       |  ");
    if (i == 0) for (j = 0; j < nr; j++) printf(1, "%d ", avail[j]);
    printf(1, "\n");
  }
  printf(1, "\n");
}

/*
 * deadlock_detect() – PART C TASK A
 * Uses the resource-allocation-graph reduction algorithm.
 * Returns number of deadlocked processes; fills out_list[].
 */
int
deadlock_detect(int np, int nr,
                int alloc[PROC_MAX][RES_MAX],
                int req[PROC_MAX][RES_MAX],
                int avail[RES_MAX],
                int out_list[PROC_MAX])
{
  int work[RES_MAX], finish[PROC_MAX];
  int i, j, found, can_run, nd = 0;

  for (j = 0; j < nr; j++) work[j] = avail[j];

  for (i = 0; i < np; i++) {
    int has_res = 0;
    for (j = 0; j < nr; j++) if (alloc[i][j] > 0) { has_res = 1; break; }
    finish[i] = (has_res == 0) ? 1 : 0;
  }

  do {
    found = 0;
    for (i = 0; i < np; i++) {
      if (finish[i]) continue;
      can_run = 1;
      for (j = 0; j < nr; j++) if (req[i][j] > work[j]) { can_run = 0; break; }
      if (can_run) {
        for (j = 0; j < nr; j++) work[j] += alloc[i][j];
        finish[i] = 1; found = 1;
      }
    }
  } while (found);

  for (i = 0; i < np; i++)
    if (!finish[i]) out_list[nd++] = i;

  return nd;
}

/*
 * safety_check() – PART C TASK B (Banker safety algorithm)
 * Returns 1 if the system is in a safe state, 0 otherwise.
 * Fills seq[] with the safe execution sequence.
 */
int
safety_check(int np, int nr,
             int alloc[PROC_MAX][RES_MAX],
             int need[PROC_MAX][RES_MAX],
             int avail[RES_MAX],
             int seq[PROC_MAX])
{
  int work[RES_MAX], finish[PROC_MAX];
  int i, j, found, can_run, cnt = 0;

  for (j = 0; j < nr; j++) work[j] = avail[j];
  for (i = 0; i < np; i++) finish[i] = 0;

  do {
    found = 0;
    for (i = 0; i < np; i++) {
      if (finish[i]) continue;
      can_run = 1;
      for (j = 0; j < nr; j++) if (need[i][j] > work[j]) { can_run = 0; break; }
      if (can_run) {
        for (j = 0; j < nr; j++) work[j] += alloc[i][j];
        finish[i] = 1; seq[cnt++] = i; found = 1;
      }
    }
  } while (found);

  return (cnt == np);
}

/*
 * try_allocate() – PART C TASK B (Banker resource-request)
 * Tentatively grants request; rolls back if state becomes unsafe.
 */
int
try_allocate(int pid, int req[RES_MAX],
             int np, int nr,
             int alloc[PROC_MAX][RES_MAX],
             int max[PROC_MAX][RES_MAX],
             int need[PROC_MAX][RES_MAX],
             int avail[RES_MAX])
{
  int j, seq[PROC_MAX];
  printf(1, "P%d requests: [ ", pid);
  for (j = 0; j < nr; j++) printf(1, "%d ", req[j]);
  printf(1, "]\n");

  for (j = 0; j < nr; j++) {
    if (req[j] > need[pid][j]) { printf(1, "ERROR: exceeds declared max need\n"); return 0; }
    if (req[j] > avail[j])     { printf(1, "WAIT:  resources unavailable\n");      return 0; }
  }

  for (j = 0; j < nr; j++) { avail[j] -= req[j]; alloc[pid][j] += req[j]; need[pid][j] -= req[j]; }

  if (safety_check(np, nr, alloc, need, avail, seq)) {
    printf(1, "GRANTED – system remains safe. Sequence: ");
    for (j = 0; j < np; j++) printf(1, "P%d%s", seq[j], j==np-1?"\n":" -> ");
    return 1;
  }

  printf(1, "DENIED – would become unsafe. Rolling back.\n");
  for (j = 0; j < nr; j++) { avail[j] += req[j]; alloc[pid][j] -= req[j]; need[pid][j] += req[j]; }
  return 0;
}

/* Simulate pipe-based circular wait and run detection on it */
void
pipe_circular_wait_demo(void)
{
  int pd1[2], pd2[2];
  int na = 2, nb = 2;
  int alloc_c[PROC_MAX][RES_MAX] = { {1,0}, {0,1} };
  int req_c[PROC_MAX][RES_MAX]   = { {0,1}, {1,0} };
  int avail_c[RES_MAX]           = { 0, 0 };
  int dl[PROC_MAX], cnt, i;

  printf(1, "\n===================================================\n");
  printf(1, "  PART C – Pipe Circular Wait Simulation           \n");
  printf(1, "===================================================\n");

  if (pipe(pd1) < 0 || pipe(pd2) < 0) { printf(1, "pipe() failed\n"); return; }

  printf(1, "P_A holds Pipe1 and waits for Pipe2\n");
  printf(1, "P_B holds Pipe2 and waits for Pipe1\n\n");
  show_state(na, nb, alloc_c, req_c, avail_c, "Waiting For");

  cnt = deadlock_detect(na, nb, alloc_c, req_c, avail_c, dl);
  if (cnt > 0) {
    printf(1, "DEADLOCK CONFIRMED between:\n");
    for (i = 0; i < cnt; i++)
      printf(1, "  Process %s (P%d)\n", dl[i]==0?"A":"B", dl[i]);
  }

  close(pd1[0]); close(pd1[1]);
  close(pd2[0]); close(pd2[1]);
}

int main(int argc, char *argv[]) {
  int i, j;
  printf(1, "\n###################################################\n");
  printf(1, "     xv6 Deadlock Detection & Avoidance Demo      \n");
  printf(1, "###################################################\n");

  /* --- Test 1: Detection --- */
  printf(1, "\n--- Test 1: Deadlocked State ---\n");
  int n1=5, r1=3;
  int a1[PROC_MAX][RES_MAX] = { {0,1,0},{2,0,0},{3,0,3},{2,1,1},{0,0,2} };
  int q1[PROC_MAX][RES_MAX] = { {0,0,0},{2,0,2},{0,0,1},{1,0,0},{0,0,2} };
  int v1[RES_MAX] = {0,1,0};
  int dl1[PROC_MAX], dc;
  show_state(n1, r1, a1, q1, v1, "Request");
  dc = deadlock_detect(n1, r1, a1, q1, v1, dl1);
  if (dc > 0) {
    printf(1, "Deadlock found: { ");
    for (i=0; i<dc; i++) printf(1, "P%d ", dl1[i]);
    printf(1, "}\n");
  }

  /* --- Test 2: Safe state --- */
  printf(1, "\n--- Test 2: Safe State (Banker) ---\n");
  int n2=5, r2=3;
  int a2[PROC_MAX][RES_MAX] = { {0,1,0},{2,0,0},{3,0,2},{2,1,1},{0,0,2} };
  int mx2[PROC_MAX][RES_MAX] = { {7,5,3},{3,2,2},{9,0,2},{2,2,2},{4,3,3} };
  int nd2[PROC_MAX][RES_MAX];
  for (i=0;i<n2;i++) for (j=0;j<r2;j++) nd2[i][j]=mx2[i][j]-a2[i][j];
  int v2[RES_MAX] = {3,3,2};
  int sq2[PROC_MAX];
  show_state(n2, r2, a2, nd2, v2, "Need");
  if (safety_check(n2, r2, a2, nd2, v2, sq2)) {
    printf(1, "System is SAFE. Sequence: ");
    for (i=0;i<n2;i++) printf(1,"P%d%s",sq2[i],i==n2-1?"\n":" -> ");
  }

  /* --- Test 3: Request that breaks safety --- */
  printf(1, "\n--- Test 3: Request Leading to Unsafe State ---\n");
  int rq3[RES_MAX] = {3,3,0};
  try_allocate(4, rq3, n2, r2, a2, mx2, nd2, v2);

  /* --- Test 4: Pipe simulation --- */
  pipe_circular_wait_demo();

  printf(1, "\n###################################################\n");
  printf(1, "              All tests completed                  \n");
  printf(1, "###################################################\n\n");
  exit();
}