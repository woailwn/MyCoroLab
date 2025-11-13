#include <iostream>
#include <coroutine>
#include <optional>
#include <string>


/*
    面向用户的对象Task
        更像是为调用者提供了一个入口来操纵协程
*/
class Task{
    public:
        class promise_type;
        using handle_type=std::coroutine_handle<promise_type>; // 协程句柄类型
        class promise_type{     //与Task关联的promise对象
            friend class Task;

        public:
            Task get_return_object(){   //用于构造协程
                auto handle = handle_type::from_promise(*this); //获取句柄
                return Task(handle);
            }

            //控制协程创建完后的调度逻辑
            constexpr std::suspend_always initial_suspend() {return {};}

            //协程返回值时会调用该函数
            void return_value(std::string result){
                m_result=result;
            }

            //协程yield会调用该函数
            std::suspend_always yield_value(int value){
                m_yield=value;
                return {};
            }

            //协程运行抛出异常时会调用该函数
            void unhandled_exception(){
                m_exception=std::current_exception();
            }

            //执行完调用该函数
            constexpr std::suspend_always final_suspend() noexcept {return {};}
            private:
                std::exception_ptr m_exception{nullptr};
                std::optional<std::string> m_result{std::nullopt};
                std::optional<int> m_yield {std::nullopt};
        };

    public:
        Task& operator=(const Task&) = delete;
        Task(Task&& other):m_handle(other.m_handle)
        {
            other.m_handle=nullptr;
        }

        Task(const Task&) = delete;
        Task& operator=(Task&& other){
            if(m_handle){
                m_handle.destroy();
            }
            m_handle=other.m_handle;
            other.m_handle=nullptr;
            return *this;
        }

        ~Task(){
            m_handle.destroy();
        }
    public:
        void resume(){
            m_handle.resume();
        }

        //用户侧不断调用next驱动协程yield值
        std::optional<int> next(){
            promise_type &p=m_handle.promise();
            p.m_yield=std::nullopt;
            p.m_exception=nullptr;
            if(!m_handle.done()){
                m_handle.resume();  //恢复协程运行
            }
            if(p.m_exception){
                std::rethrow_exception(p.m_exception);
            }
            return p.m_yield;
        }

        //用户手动调用该函数获取协程返回值
        std::optional<std::string> result(){
            return m_handle.promise().m_result;
        }
    private:
        Task(handle_type handle):m_handle(handle){}
    private:
        handle_type m_handle;
};

/*
    协程函数
*/
Task run(int i){
    std::cout<<"task "<<i<<" start"<<std::endl;
    co_yield 1;
    for(int i=2;i<=5;++i){
        co_yield i;
    }
    std::cout<<"task "<<i<<" end"<<std::endl;
    co_return "task run finish";
}

int main(){
    auto task=run(5);
    std::optional<int> val;
    while((val=task.next())){
        std::cout<<"get yield value: "<<(*val)<<std::endl;
    }
    std::cout<<"get return value: "<<(*task.result())<<std::endl;
}