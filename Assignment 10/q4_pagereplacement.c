// Q4 main test: compare FIFO, LRU, and Optimal replacement algorithms.
#include "types.h"
#include "stat.h"
#include "user.h"

#define NREF 20
#define MAXFRAMES 5

static int refs[NREF] = {7,0,1,2,0,3,0,4,2,3,0,3,2,1,2,0,1,7,0,1};

enum { FIFO, LRU, OPTIMAL };
static char *names[] = {"FIFO", "LRU", "Optimal"};

static void
showframes(int *frames, int nframes)
{
  int i;
  printf(1, "[");
  for(i = 0; i < nframes; i++){
    if(frames[i] < 0)
      printf(1, " -");
    else
      printf(1, " %d", frames[i]);
  }
  printf(1, " ]");
}

static int
simulate(int algorithm, int nframes, int trace)
{
  int frames[MAXFRAMES];
  int loaded[MAXFRAMES];
  int used[MAXFRAMES];
  int i, j, hit, victim, faults = 0;

  for(j = 0; j < nframes; j++){
    frames[j] = -1;
    loaded[j] = 0;
    used[j] = 0;
  }

  for(i = 0; i < NREF; i++){
    hit = -1;
    for(j = 0; j < nframes; j++)
      if(frames[j] == refs[i]){
        hit = j;
        break;
      }

    if(hit >= 0){
      used[hit] = i + 1;
    } else {
      faults++;
      victim = -1;
      for(j = 0; j < nframes; j++)
        if(frames[j] < 0){
          victim = j;
          break;
        }

      if(victim < 0 && algorithm == FIFO){
        victim = 0;
        for(j = 1; j < nframes; j++)
          if(loaded[j] < loaded[victim])
            victim = j;
      } else if(victim < 0 && algorithm == LRU){
        victim = 0;
        for(j = 1; j < nframes; j++)
          if(used[j] < used[victim])
            victim = j;
      } else if(victim < 0){
        int farthest = -1;
        for(j = 0; j < nframes; j++){
          int next;
          for(next = i + 1; next < NREF; next++)
            if(refs[next] == frames[j])
              break;
          if(next > farthest){
            farthest = next;
            victim = j;
          }
        }
      }

      frames[victim] = refs[i];
      loaded[victim] = i + 1;
      used[victim] = i + 1;
    }

    if(trace){
      printf(1, "ref %d: ", refs[i]);
      showframes(frames, nframes);
      printf(1, hit >= 0 ? "  HIT\n" : "  FAULT\n");
    }
  }
  return faults;
}

static void
print_ratio(int count)
{
  int ratio = count * 10000 / NREF;
  printf(1, "%d.", ratio / 100);
  if(ratio % 100 < 10)
    printf(1, "0");
  printf(1, "%d%%", ratio % 100);
}

static void
print_table(void)
{
  int frames, algorithm, faults;
  printf(1, "\nFrames Algorithm Faults Hits Hit ratio Fault ratio\n");
  printf(1, "------ --------- ------ ---- --------- -----------\n");
  for(frames = 3; frames <= 5; frames++){
    for(algorithm = FIFO; algorithm <= OPTIMAL; algorithm++){
      faults = simulate(algorithm, frames, 0);
      printf(1, "  %d    %s %d %d ", frames, names[algorithm],
             faults, NREF - faults);
      print_ratio(NREF - faults);
      printf(1, " ");
      print_ratio(faults);
      printf(1, "\n");
    }
  }
}

int
main(void)
{
  int algorithm, faults;

  printf(1, "Q4: FIFO, LRU, and Optimal page replacement\n");
  printf(1, "Reference string:");
  for(int i = 0; i < NREF; i++)
    printf(1, " %d", refs[i]);
  printf(1, "\n\n");

  for(algorithm = FIFO; algorithm <= OPTIMAL; algorithm++){
    printf(1, "--- %s, 3 frames ---\n", names[algorithm]);
    faults = simulate(algorithm, 3, 1);
    printf(1, "%s page faults: %d; hits: %d\n\n",
           names[algorithm], faults, NREF - faults);
  }

  print_table();
  exit();
}
