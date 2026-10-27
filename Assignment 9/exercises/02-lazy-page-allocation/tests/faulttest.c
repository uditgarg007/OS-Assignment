// Assignment 9 Q2: test program for lazy allocation.
#include "types.h"
#include "stat.h"
#include "user.h"

#define PG 4096

int
main(int argc, char *argv[])
{
  char *p;
  int i, pid, f0, faults;
  int pages[5] = {0, 3, 7, 12, 31};

  f0 = getfaults();
  p = sbrk(32 * PG);   // lazy: nothing is allocated yet
  printf(1, "sbrk(32 * 4096) returned 0x%x, faults so far: %d\n", p, getfaults() - f0);

  for(i = 0; i < 5; i++)
    p[pages[i] * PG] = 'x';   // first touch of 5 distinct pages

  faults = getfaults() - f0;
  printf(1, "touched 5 pages, page faults handled: %d\n", faults);
  if(faults == 5)
    printf(1, "PASS: result is 5\n");
  else
    printf(1, "FAIL: expected 5\n");

  // Touch a page again: already mapped, no new fault.
  p[0] = 'y';
  printf(1, "re-touch of mapped page, total faults: %d\n", getfaults() - f0);

  // Access beyond the allocated size: only that process must die.
  printf(1, "child will now access address above its size...\n");
  pid = fork();
  if(pid == 0){
    p[40 * PG] = 'z';   // 32 pages were allocated, page 40 is invalid
    printf(1, "FAIL: child survived an invalid access\n");
    exit();
  }
  wait();
  printf(1, "parent still alive after child was killed: PASS\n");
  exit();
}
