#include "types.h"
#include "stat.h"
#include "user.h"

#define PAGE_SZ       4096
#define THREAD_COUNT  3
#define ITER_COUNT    100000

/* Shared variable – visible to all threads in the same address space */
volatile int global_ctr = 0;

/* Thread body: increments global_ctr ITER_COUNT times */
void thread_body(void *arg) {
  int tid = (int)(long)arg;
  printf(1, "[T%d] starting: will do %d iterations on global_ctr\n", tid, ITER_COUNT);
  int i;
  for (i = 0; i < ITER_COUNT; i++) {
    /* No lock — races are intentional to demonstrate the issue */
    global_ctr++;
  }
  printf(1, "[T%d] done. Calling exit().\n", tid);
  exit();
}

int main(int argc, char *argv[]) {
  printf(1, "\n===================================================\n");
  printf(1, "  xv6 clone/join Thread Test: Shared Memory Demo  \n");
  printf(1, "===================================================\n");
  printf(1, "global_ctr initial = %d\n", global_ctr);
  printf(1, "Launching %d threads x %d iters each...\n", THREAD_COUNT, ITER_COUNT);
  printf(1, "Theoretical max = %d\n\n", THREAD_COUNT * ITER_COUNT);

  void *stk[THREAD_COUNT];
  int  tid_arr[THREAD_COUNT];
  int i;

  for (i = 0; i < THREAD_COUNT; i++) {
    stk[i] = malloc(PAGE_SZ);
    if (!stk[i]) { printf(1,"malloc failed\n"); exit(); }
    void *stk_top = (char*)stk[i] + PAGE_SZ;
    tid_arr[i] = clone(thread_body, (void*)(long)(i+1), stk_top);
    if (tid_arr[i] < 0) { printf(1,"clone failed\n"); exit(); }
    printf(1, "[main] spawned thread %d => pid %d\n", i+1, tid_arr[i]);
  }

  /* Reap all threads */
  for (i = 0; i < THREAD_COUNT; i++) {
    int reaped = join();
    printf(1, "[main] joined pid %d\n", reaped);
    free(stk[i]);
  }

  int expected = THREAD_COUNT * ITER_COUNT;
  int lost = expected - global_ctr;
  printf(1, "\n---------------------------------------------------\n");
  printf(1, "Final global_ctr : %d\n", global_ctr);
  printf(1, "Expected         : %d\n", expected);
  printf(1, "Lost updates     : %d  (due to unsynchronised access)\n", lost);
  printf(1, "---------------------------------------------------\n\n");

  if (global_ctr < expected)
    printf(1, "Race condition confirmed: shared memory visible across threads.\n");
  else
    printf(1, "No race observed this run.\n");

  exit();
}