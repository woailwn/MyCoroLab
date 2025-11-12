#include <cstdlib>
#include <iostream>
#include <memory>
#include <ucontext.h>

using namespace std;

/*
执行流程切换图
main(ctx0) coroutine1  coroutine2
 |
 |
 |---------->|
             |
             |---------->|
                         |
                         |
             |<----------|
             |
             |---------->|
                         |
             |<----------|
             |
  |<---------|
  |end
 */

ucontext_t ctx0,ctx1,ctx2;

void coroutine1(){
    cout<<"Hi,this is coroutine1"<<endl;
    cout<<"Now I will switch to corotine2"<<endl;
    swapcontext(&ctx1,&ctx2);
    cout<<"Welcome back to coroutine1"<<endl;
    cout<<"Now I will switch to coroutine2 again"<<endl;
    swapcontext(&ctx1,&ctx2);
    setcontext(&ctx0);
}

void coroutine2(){
    cout<<"Hi,this is coroutine2"<<endl;
    cout<<"Now I will switch to corotine1"<<endl;
    swapcontext(&ctx2,&ctx1);
    cout<<"Welcome back to coroutine2"<<endl;
    cout<<"Now I will switch to coroutine1 again"<<endl;
    swapcontext(&ctx2,&ctx1);
}

int main(){
    ctx0.uc_link=nullptr;
    ctx0.uc_stack.ss_sp=malloc(1024);
    ctx0.uc_stack.ss_size=1024;
    ctx0.uc_link=NULL;

    ctx1.uc_link=nullptr;
    ctx1.uc_stack.ss_sp=malloc(1024);
    ctx1.uc_stack.ss_size=1024;

    //如果想回到main 那就设置为&ctx0
    ctx1.uc_link=NULL;

    ctx2.uc_link=nullptr;
    ctx2.uc_stack.ss_sp=malloc(1024);
    ctx2.uc_stack.ss_size=1024;
    ctx2.uc_link=NULL;

    getcontext(&ctx1);
    makecontext(&ctx1,coroutine1,0);

    getcontext(&ctx2);
    makecontext(&ctx2,coroutine2,0);
    
    cout<<"Hi,this is coroutine0"<<endl;
    cout<<"Now I will switch to coroutine1"<<endl;

    swapcontext(&ctx0,&ctx1);

    cout<<"Welcome back to coroutine0"<<endl;

    free(ctx0.uc_stack.ss_sp);
    free(ctx1.uc_stack.ss_sp);
    free(ctx2.uc_stack.ss_sp);

    cout<<"finish all jobs"<<endl;
    return 0;
}