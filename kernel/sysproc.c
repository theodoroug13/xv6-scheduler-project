#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "vm.h"
#include "pstat.h"

extern struct proc proc[NPROC];
extern struct spinlock wait_lock;

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  kexit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return kfork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return kwait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int t;
  int n;

  argint(0, &n);
  argint(1, &t);
  addr = myproc()->sz;

  if(t == SBRK_EAGER || n < 0) {
    if(growproc(n) < 0) {
      return -1;
    }
  } else {
    // Lazily allocate memory for this process: increase its memory
    // size but don't allocate memory. If the processes uses the
    // memory, vmfault() will allocate it.
    if(addr + n < addr)
      return -1;
    if(addr + n > TRAPFRAME)
      return -1;
    myproc()->sz += n;
  }
  return addr;
}

uint64
sys_pause(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if(n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kkill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

//helper to turn state into a string
static void
state_to_str(enum procstate st, char *out)
{
  switch(st){
  case UNUSED:safestrcpy(out, "UNUSED", PSTAT_STATELEN); 
    break;
  case USED:safestrcpy(out, "USED", PSTAT_STATELEN);
    break;
  case SLEEPING: safestrcpy(out, "SLEEPING", PSTAT_STATELEN); 
    break;
  case RUNNABLE: safestrcpy(out, "RUNNABLE", PSTAT_STATELEN);
    break;
  case RUNNING: safestrcpy(out, "RUNNING", PSTAT_STATELEN); 
    break;
  case ZOMBIE: safestrcpy(out, "ZOMBIE", PSTAT_STATELEN);
    break;
  default:safestrcpy(out, "?", PSTAT_STATELEN);
    break;
  }
}


// return info about every active process

uint64
sys_getpinfo(void)
{
  uint64 useraddr;
  argaddr(0,&useraddr);
  
  struct pstat st;
  memset(&st, 0, sizeof(st));

  acquire(&wait_lock);
  for( int i=0;i<NPROC;i++){
    struct proc *p=&proc[i];
    acquire(&p->lock);
    if(p->state!=UNUSED){
      st.pid[i]=p->pid;
      st.inuse[i]=1;
      if (p->parent){
        st.ppid[i]=p->parent->pid;
      }
      else{
        st.ppid[i]=0;
      }
      safestrcpy(st.name[i],p->name,PSTAT_NAMELEN);
      state_to_str(p->state,st.state[i]); 
      st.sz[i]=p->sz;

      st.priority[i]=0;
        
    
    }

    release(&p->lock);

  }
  release(&wait_lock);
  if(copyout(myproc()->pagetable,useraddr, (char*)&st, sizeof(st))<0){
    printf("copyout error");
    return -1;
  }
  return 0;
}
