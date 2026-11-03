// Q2 main test: translate mapped addresses and reject an unmapped address.
#include "types.h"
#include "stat.h"
#include "user.h"

int
main(void)
{
  char *mem;
  int va, pa;

  printf(1, "Q2: virtual-to-physical address translation\n");
  mem = sbrk(4096);
  if(mem == (char*)-1){
    printf(1, "q2_vtop: sbrk failed\n");
    exit();
  }
  mem[0] = 'x';
  mem[4095] = 'y';

  va = (int)mem;
  pa = vtop(va);
  printf(1, "Mapped address:   VA=0x%x -> PA=0x%x\n", (uint)va, (uint)pa);
  va++;
  pa = vtop(va);
  printf(1, "Address + 1 byte: VA=0x%x -> PA=0x%x\n", (uint)va, (uint)pa);

  va = 0x7ffff000;
  pa = vtop(va);
  printf(1, "Unmapped address: VA=0x%x -> result=%d (expected -1)\n",
         (uint)va, pa);
  exit();
}
