#include "kernel/types.h"
#include "kernel/param.h"
#include "kernel/pstat.h"
#include "user.h"

int main(void){
    
    struct pstat st;
    if(getpinfo(&st)<0){
        fprintf(2, "getpinfo failed");
        exit(1);
    }
    printf("PID\tPPID\tPRIO\tSTATE\t\tSZ\tNAME\n");
    for (int i=0;i<NPROC;i++){
        if(st.inuse[i]){
            printf("%d\t%d\t%d\t%s\t%lu\t%s\n",st.pid[i],st.ppid[i], st.priority[i], st.state[i],st.sz[i],st.name[i]);
        }
    }
    exit(0);

} 