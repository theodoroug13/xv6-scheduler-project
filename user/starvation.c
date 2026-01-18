#include "kernel/types.h"
#include "kernel/param.h"
#include "kernel/pstat.h"
#include "user/user.h"

static int get_level(int pid){
  struct pstat st;
  if(getpinfo(&st) < 0) return -1;
  for(int i=0;i<NPROC;i++){
    if(st.inuse[i] && st.pid[i]==pid) return st.priority[i];
  }
  return -1;
}

static void burn_ticks(int ticks){
  int t0 = uptime();
  volatile int x=0;
  while(uptime()-t0 < ticks) x++;
}

// pressure: keep a lot of level-0 runnable work around
static void pressure(void){
  for(;;){
    for(int i=0;i<8;i++){
      int c=fork();
      if(c==0){
        burn_ticks(2);
        exit(0);
      }
    }
    for(int i=0;i<8;i++) wait(0);
  }
}

int main(void){
  int victim = fork();
  if(victim == 0){
    // Phase A: get demoted to level 3 (no pressure yet)
    burn_ticks(400);
    // Then stay RUNNABLE forever
    for(;;){ volatile int y=0; y++; }
  }

  // Monitor until we SEE level 3 (Phase A confirmation)
  int startA = uptime();
  while(uptime() - startA < 600){
    int lvl = get_level(victim);
    if(lvl >= 0){
      printf("[mfbst] A t=%d victim level=%d\n", uptime(), lvl);
      if(lvl == 3) break;
    }
    pause(10);
  }

  int lvlA = get_level(victim);
  if(lvlA != 3){
    fprintf(2, "[mfbst] FAIL: victim never reached level 3 in phase A\n");
    kill(victim);
    exit(1);
  }

  // Phase B: start pressure AFTER victim is already at level 3
  int p1 = fork();
  if(p1 == 0) pressure();
  int p2 = fork();
  if(p2 == 0) pressure();

  int startB = uptime();
  while(uptime() - startB < 1200){
    int lvl = get_level(victim);
    if(lvl >= 0){
      printf("[mfbst] B t=%d victim level=%d\n", uptime(), lvl);
      if(lvl <= 2){
        printf("[mfbst] PASS: promotion observed (3 -> %d)\n", lvl);
        kill(p1); kill(p2); kill(victim);
        exit(0);
      }
    }
    pause(10);
  }

  fprintf(2, "[mfbst] FAIL: no promotion observed from level 3\n");
  kill(p1); kill(p2); kill(victim);
  exit(1);
}
