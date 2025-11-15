promise,协程句柄,协程状态
co_await,co_return,co_yield

协程帧的结构
![Alt text](image.png)

包含promise_type的类统称为UserFacing
coroutine_handle:外部用以操作协程的句柄
1.promise与coroutine_handle可以互相转化
2.拿到协程句柄操作:
* handle.promise() 拿到promise
* handle.done()    判断协程是否执行结束
* handle.resume()  让暂停的协程继续运行
* handle.destory() 回收协程栈


promise:协程的核心
1.构造:调用协程时，编译器找promise_type对应的promise构造在协程帧上(promise可以有多个构造函数)

2.get_return_object - 获取的对象(包含promise的类)是通过它构造出来的,参数为空
                      UserFacing会将coroutine_handle作为构造函数的参数，就可以访问promise的数据,这个函数协程函数调用时被调用

3.initial_suspend   - 协程创建时的调度点   suspend_always(暂停) suspend_never(继续执行)

4.final_suspend     - 负责协程结束后的调度点逻辑,返回同样是awaiter类型(形如suspend_always这些...)

5.co_return & return_value  
(1)co_return不返回值,仅用于终止协程运行,编译器会调用promise.return_void方法,可以执行一些协程结束后的清理工作
(2)co_return有返回值,编译器调用promise.return_value方法,并将co_return的返回值作为参数传入

6.co_yield & yield_value
与co_return类似
但是,co_yield之后的协程运行并不一定结束,所以yield_value会返回awaiter类型来决定协程的执行权如何处理
可以用optional<T>保存co_yield的值便于协程调用方判断迭代循环是否结束

7.unhandled_exception
捕获协程中抛出的异常,没有参数,此时控制权交给协程调用者，用户可在UserFacing获取存储的异常，并再次抛出异常




Awaiter:调度点的具体逻辑
要实现下面的方法
* await_ready
* await_suspend
* await_resume

co_await:awaiter执行的触发器
后面跟awaiter对象
(这里还有点细节没懂)

await_ready:当执行co_await awaiter时,编译器先会执行awaiter.await_ready方法(返回值为true)->协程就绪，继续运行而非暂停 false->await_suspend
bool awaiter::await_ready();

await_suspend
void awaiter::await_suspend(std::coroutine_handle<>);   ->协程停止，转移执行权
bool awaiter::await_suspend(std::coroutine_handle<>);   ->true则暂停协程，转移执行权，否则协程继续运行
std::coroutine_handle<> awaiter::await_suspend(std::coroutine_handle<>);    ->返回的协程句柄会被编译器隐式调用resume，即该句柄关联的协程会继续运行
await_ready->false->调用await_suspend

await_resume
T awaiter::await_resume()

问题:
协程设计为什么要将写成参数列表与promise构造函数关联?
a:传递this指针
