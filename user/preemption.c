#include "kernel/types.h"
#include "kernel/param.h"
#include "kernel/pstat.h"
#include "user/user.h"

static int
get_level(int pid)
{
  struct pstat st;
  if(getpinfo(&st) < 0) return -1;
  for(int i = 0; i < NPROC; i++){
    if(st.inuse[i] && st.pid[i] == pid)
      return st.priority[i];
  }
  return -1;
}

// Consume exactly ~1 tick of CPU time (best-effort)
static void
burn_one_tick(void)
{
  int t0 = uptime();
  volatile int x = 0;
  while(uptime() == t0){
    x++;
  }
}

int
main(void)
{
  int pid = fork();
  if(pid < 0){
    fprintf(2, "fork failed\n");
    exit(1);
  }

  if(pid == 0){
    // Do 8 times: burn ~1 tick, then sleep 1 tick.
    // If quantum resets on "giving up CPU", it may never demote.
    for(int r = 0; r < 8; r++){
      burn_one_tick();
      pause(1);
    }
    // keep running a bit more to make result visible
    for(int r = 0; r < 20; r++){
      burn_one_tick();
      pause(1);
    }
    exit(0);
  }

  int saw_level1 = 0;
  for(int k = 0; k < 60; k++){
    int lvl = get_level(pid);
    if(lvl >= 0){
      printf("[noreset] t=%d pid=%d level=%d\n", uptime(), pid, lvl);
      if(lvl >= 1) saw_level1 = 1;
    }
    pause(5);
  }

  wait(0);

  if(!saw_level1){
    fprintf(2, "[noreset] FAIL: process never demoted to level>=1 (quantum may be resetting)\n");
    exit(1);
  }

  printf("[noreset] PASS\n");
  exit(0);
}
