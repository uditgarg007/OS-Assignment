// Assignment 9 Q1: test program for the vmprint() system call.
#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  int before, after;

  printf(1, "=== vmprint at start-up ===\n");
  before = vmprint();

  sbrk(4 * 4096);

  printf(1, "=== vmprint after sbrk(4 * 4096) ===\n");
  after = vmprint();

  printf(1, "valid user pages before: %d, after: %d, new: %d\n",
         before, after, after - before);
  if(after - before == 4)
    printf(1, "PASS: exactly four new pages appeared\n");
  else
    printf(1, "FAIL: expected 4 new pages\n");
  exit();
}
