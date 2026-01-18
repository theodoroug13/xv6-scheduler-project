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
      return st.priority[i]; // == mlfq_level
  }
  return -1;
}

static void
burn_for_ticks(int ticks)
{
  int t0 = uptime();
  volatile int x = 0;
  while(uptime() - t0 < ticks){
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
    // CPU hog for long enough to demote down to 3
    burn_for_ticks(300);
    exit(0);
  }

  int last = -1;
  int reached3 = 0;

  for(int k = 0; k < 40; k++){
    int lvl = get_level(pid);
    if(lvl >= 0 && lvl != last){
      printf("[demotion] t=%d pid=%d level=%d\n", uptime(), pid, lvl);
      last = lvl;
      if(lvl == 3) reached3 = 1;
    }
    pause(10);
  }

  wait(0);

  if(!reached3){
    fprintf(2, "[demotion] FAIL: never reached level 3\n");
    exit(1);
  }

  printf("[demotion] PASS\n");
  exit(0);
}
