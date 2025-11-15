#include <coroutine>
#include <iostream>

std::coroutine_handle<> global_handle;

class Event
{
public:
    Event() { std::cout << "event construct" << std::endl; }
    ~Event() { std::cout << "event deconstruct" << std::endl; }

    // co_await后 是否继续运行协程
    bool await_ready() { return false; }
    // 暂停又马上恢复
    std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller) { return caller; }
    // 恢复后的行为
    void await_resume()
    {
        std::cout << "await resume called" << std::endl;
    }
};

class Task
{
public:
    class promise_type;
    using handle_type = std::coroutine_handle<promise_type>;
    class promise_type
    {
        friend class Task;

    public:
        promise_type(int id) : m_id(id) {}
        Task get_return_object()
        {
            auto handle = handle_type::from_promise(*this);
            global_handle = handle;
            return Task(handle, m_id);
        }

        constexpr std::suspend_never initial_suspend() { return {}; } // 创建继续执行

        void return_void() {}

        void unhandled_exception() {}

        constexpr std::suspend_always final_suspend() noexcept { return {}; }

    private:
        int m_id;
    };

public:
    Task(const Task &) = delete;
    Task(Task &&other) : m_handle(other.m_handle)
    {
        other.m_handle = nullptr;
    }
    Task &operator=(const Task &other) = delete;
    Task &operator=(Task &&other)
    {
        if (m_handle)
        {
            m_handle.destroy();
        }
        m_handle = other.m_handle;
        other.m_handle = nullptr;
        return *this;
    }

    ~Task()
    {
        std::cout << "deconstruct task " << m_id << std::endl;
    }

public:
    auto operator co_await()
    {
        return Event{};
    }

    void resume()
    {
        m_handle.resume();
    }

private:
    Task(handle_type handle, int id) : m_handle(handle), m_id(id)
    {
        std::cout << "construct task " << m_id << std::endl;
    }

private:
    int m_id;
    handle_type m_handle;
};

Task run(int i)
{
    std::cout << "task " << i << " start" << std::endl;
    if (i == 0)
    {
        co_await run(i + 1);
    }
    else
    {
        std::cout << "task " << i << " will suspend" << std::endl;
        co_await std::suspend_always{};
    }

    std::cout << "task " << i << " will suspend" << std::endl;
    co_await std::suspend_always{};

    std::cout << "task " << i << " end" << std::endl;
    co_return;
}

int main()
{
    auto task = run(0);
    std::cout << "back to main" << std::endl;
    global_handle.resume();
    std::cout << "run finish" << std::endl;
    return 0;
}

/*
    调用流程
        main(run(0))
        --->task 0(get_return_object)
            ⬇
            co_await --------------------> run(1) (get_return_object)
                                                  ⬇
    返回Task,构造Event对象 <-------------------  co_await suspend_always
                  ⬇
      main <---- task 0(co_await)
     resume    --------------------------------> task1
                                                    ⬇
                                                 co_await std::suspend_always


    几个问题:
    1.Event什么时候被创建?
    co_await时被创建,进行一些操作

    2.为什么co_await返回涉及Event?
    4.为什么co_await std::suspend_always不会构造event
    我们的Task对象重载了 co_await
    auto operator co_await(){
        return Event{};
    }

            co_await expression
                    ↓
            检查 expression 的类型
                    ↓
        ┌─────────────────────────────────┐
        │ 有 operator co_await() 吗？    
        └─────────────────────────────────┘
            ↓ 有              ↓ 没有
        调用它获取 awaiter   检查是否本身是 awaiter
            ↓                    ↓
        返回的对象作为       直接用自己作为
            awaiter              awaiter

    再有一个原因就是 suspend_always本身就实现了awaiter接口
    struct suspend_always {
        // 本身就实现了 awaiter 接口
        bool await_ready() const noexcept { return false; }
        void await_suspend(coroutine_handle<>) const noexcept {}
        void await_resume() const noexcept {}
    
        // 没有 operator co_await()
    };
    
    3.为什么global_handle.resume()恢复的是task1?
    global_handle是全局变量，进入task1时被覆盖了
    
*/