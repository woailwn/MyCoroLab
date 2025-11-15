//协程如何跨线程执行
#include <coroutine>
#include <iostream>
#include <stdexcept>
#include <thread>

struct task
{
  struct promise_type
  {
    task get_return_object() { return {}; }
    std::suspend_never initial_suspend() { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void return_void() {}
    void unhandled_exception() {}
  };
};

//jthread析构时自动join
auto switch_to_new_thread(std::jthread &out){
    struct awaitable{
        std::jthread *p_out;
        bool await_ready() {return false;}
        void await_suspend(std::coroutine_handle<> h){
            std::cout<<"旧线程 ID: "<<std::this_thread::get_id()<<std::endl;
            std::jthread &out=*p_out;
            if(out.joinable())
                throw std::runtime_error("jthread 输出参数非空");
            out = std::jthread([h]{h.resume();});   //一定可以等到协程完成 因为join了
            std::cout<<"新线程 ID: "<<out.get_id()<<std::endl;
        }
        void await_resume(){}
    };
    return awaitable{&out}; //自定义awaiter
}

task resuming_on_new_thread (std::jthread &out){
    std::cout<<"协程开始,线程 ID: "<<std::this_thread::get_id()<<std::endl;
    co_await switch_to_new_thread(out);
    std::cout<<"协程恢复,线程 ID: "<<std::this_thread::get_id()<<std::endl;
}

int main(){
    std::jthread out;
    resuming_on_new_thread(out);
}

