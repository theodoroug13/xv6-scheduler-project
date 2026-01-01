#ifndef _PSTAT_H_    
#define _PSTAT_H_

#include "param.h"
#include "types.h"

#define PSTAT_NAMELEN 16
#define PSTAT_STATELEN 16

struct pstat{
    int pid[NPROC];
    int ppid[NPROC];
    int inuse[NPROC];
    char name[NPROC][PSTAT_NAMELEN];
    char state[NPROC][PSTAT_STATELEN];
    uint64 sz[NPROC];

    //για MLFQ
    int priority[NPROC];

}

#endif