#include <ucontext.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <iostream>

int main(){
    struct timespec start,end;
    clock_gettime(CLOCK_MONOTONIC,&start);

    int idx=0;
    ucontext_t ctx1;
    getcontext(&ctx1);

    idx++;
    if(idx==1){
        setcontext(&ctx1);
    }
    clock_gettime(CLOCK_MONOTONIC,&end);

    double time_spent=(start.tv_sec-end.tv_sec) + (start.tv_nsec-end.tv_nsec)/1e9;
    std::cout<<"Ucontext switch time: "<<time_spent<<" seconds"<<std::endl;
    return 0;
}
