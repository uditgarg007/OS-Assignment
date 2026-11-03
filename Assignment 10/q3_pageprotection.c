// Q3 main test: inspect a page, remove write permission, and trigger a fault.
#include "types.h"
#include "stat.h"
#include "user.h"

int
main(void)
{
  char *page;
  int pid;

  printf(1, "Q3: page information and read-only protection\n");
  page = sbrk(4096);
  if(page == (char*)-1){
    printf(1, "q3_pageprotection: sbrk failed\n");
    exit();
  }
  page[0] = 'R';
  printf(1, "Before protection:\n");
  if(pageinfo((int)page) < 0){
    printf(1, "q3_pageprotection: pageinfo failed\n");
    exit();
  }
  if(setreadonly((int)page) < 0){
    printf(1, "q3_pageprotection: setreadonly failed\n");
    exit();
  }
  printf(1, "After setreadonly:\n");
  if(pageinfo((int)page) < 0){
    printf(1, "q3_pageprotection: pageinfo failed after protection\n");
    exit();
  }

  pid = fork();
  if(pid < 0){
    printf(1, "q3_pageprotection: fork failed\n");
    exit();
  }
  if(pid == 0){
    printf(1, "Child: attempting write to the read-only page...\n");
    *(volatile char*)page = 'W';
    printf(1, "Child: unexpected write success\n");
    exit();
  }
  wait();
  printf(1, "Parent: child was terminated by the expected write page fault.\n");
  exit();
}
