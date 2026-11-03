// Q1 main test: allocate and release physical pages through sbrk().
#include "types.h"
#include "stat.h"
#include "user.h"

int
main(void)
{
  int pages = 3;
  int bytes = pages * 4096;
  char *mem;

  printf(1, "Q1: physical page allocation statistics\n");
  printf(1, "Before allocation:\n");
  memstats();

  mem = sbrk(bytes);
  if(mem == (char*)-1){
    printf(1, "q1_memstats: sbrk allocation failed\n");
    exit();
  }
  for(int i = 0; i < pages; i++)
    mem[i * 4096] = (char)i;

  printf(1, "After allocating and touching %d pages:\n", pages);
  memstats();

  if(sbrk(-bytes) == (char*)-1){
    printf(1, "q1_memstats: sbrk release failed\n");
    exit();
  }
  printf(1, "After releasing %d pages:\n", pages);
  memstats();
  exit();
}
