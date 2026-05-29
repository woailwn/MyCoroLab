#include <string>
#include <vector>

#include "coro/task.hpp"
#include "gtest/gtest.h"

using namespace coro;

/*************************************************************
 *                       pre-definition                      *
 *************************************************************/

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

class TaskTest : public ::testing::Test
{
protected:
    void SetUp() override {}

    void TearDown() override {}

    std::vector<int> vec; // use vector to record state when task suspend
};

task<> func0()
{
    co_return;
}

task<> func1(std::vector<int>& vec)
{
    vec.push_back(1);
    co_return;
}

task<> func2(std::vector<int>& vec)
{
    vec.push_back(1);
    co_await std::suspend_always{};
    vec.push_back(2);
    co_return;
}

task<int> func3()
{
    co_return 1;
}

task<std::string> func4()
{
    co_return "tinycoro";
}

task<int> func5()
{
    auto result = co_await func3();
    co_return result + 2;
}

task<int> func6(int value)
{
    if (value == 1)
    {
        co_return 1;
    }
    auto p = co_await func6(value - 1);
    co_return p + value;
}

task<int> fibonacci(int value)
{
    if (value == 1)
    {
        co_return 0;
    }
    else if (value == 2)
    {
        co_return 1;
    }
    else
    {
        co_return (co_await fibonacci(value - 1)) + (co_await fibonacci(value - 2));
    }
}

task<int> func7(int value, std::vector<int>& vec)
{
    if (value == 1)
    {
        co_return 1;
    }
    auto p = co_await func7(value - 1, vec);
    if (value == 5)
    {
        vec.push_back(p);
        co_await std::suspend_always{};
    }
    co_return p + value;
}

/*************************************************************
 *                          tests                            *
 *************************************************************/

// task should destroy handler by itself, otherwise memory leaks.
TEST_F(TaskTest, SelfDestroy)
{
    auto p = func0();
    ASSERT_TRUE(true);
}

// test a simple task running case.
TEST_F(TaskTest, VoidTaskCase1)
{
    auto p = func1(vec);
    p.resume();
    ASSERT_EQ(vec[0], 1);
}

// test a simple task running case, which will suspend once.
TEST_F(TaskTest, VoidTaskCase2)
{
    auto p = func2(vec);
    p.resume();
    ASSERT_EQ(vec[0], 1);
    p.resume();
    ASSERT_EQ(vec.size(), 2);
    ASSERT_EQ(vec[1], 2);
}

// test the move constructor of task.
TEST_F(TaskTest, VoidTaskMove1)
{
    auto p1 = func1(vec);
    auto p2 = std::move(p1);
    p2.resume();
    ASSERT_EQ(vec[0], 1);
    ASSERT_EQ(p1.handle(), nullptr);
}

// test the move constructor of task, which will suspend once.
TEST_F(TaskTest, VoidTaskMove2)
{
    auto p1 = func2(vec);
    p1.resume();
    ASSERT_EQ(vec[0], 1);
    auto p2 = std::move(p1);
    p2.resume();
    ASSERT_EQ(vec.size(), 2);
    ASSERT_EQ(vec[1], 2);
}

// test the move assignment operator of task, which will suspend once.
TEST_F(TaskTest, VoidTaskMove3)
{
    auto p1 = func2(vec);
    p1.resume();
    ASSERT_EQ(vec[0], 1);
    auto p2 = func2(vec);
    p2      = std::move(p1);
    ASSERT_EQ(p1.handle(), nullptr);
    p2.resume();
    ASSERT_EQ(vec[1], 2);
}

// test the detach func of task.
TEST_F(TaskTest, VoidTaskDetach1)
{
    auto p = func2(vec);
    p.resume();
    ASSERT_EQ(vec[0], 1);
    auto handle = p.handle();
    p.detach();
    ASSERT_EQ(p.handle(), nullptr);
    handle.resume();
    ASSERT_EQ(vec.size(), 2);
    ASSERT_EQ(vec[1], 2);
    handle.destroy();
}

// test the clean func when task isn't detached.
TEST_F(TaskTest, VoidTaskDetach2)
{
    auto p      = func0();
    auto handle = p.handle();
    clean(handle);
    ASSERT_TRUE(true);
}

// test the clean func when task is detached.
TEST_F(TaskTest, VoidTaskDetach3)
{
    auto p      = func0();
    auto handle = p.handle();
    p.detach();
    clean(handle);
    ASSERT_TRUE(true);
}

// test a simple task running case which returns a number.
TEST_F(TaskTest, ValueTaskCase1)
{
    auto p = func3();
    p.resume();
    ASSERT_EQ(p.promise().result(), 1);
}

// test a simple task running case which returns a string.
TEST_F(TaskTest, ValueTaskCase2)
{
    const std::string str{"tinycoro"};

    auto p = func4();
    p.resume();
    ASSERT_EQ(p.promise().result(), str);
}

// test a simple task running case which calls other task.
TEST_F(TaskTest, SubcallCase1)
{
    auto p = func5();
    p.resume();
    ASSERT_EQ(p.promise().result(), 3);
}

// test the sum func implemented by task nested call.
TEST_F(TaskTest, SubcallCase2)
{
    auto p = func6(4);
    p.resume();
    ASSERT_EQ(p.promise().result(), 10);
}

// test the sum func implemented by task nested call.
TEST_F(TaskTest, SubcallCase3)
{
    auto p = func6(100);
    p.resume();
    ASSERT_EQ(p.promise().result(), 5050);
}

// test the fibonacci func implemented by task nested call.
TEST_F(TaskTest, SubcallCase4)
{
    auto p = fibonacci(10);
    p.resume();
    ASSERT_EQ(p.promise().result(), 34);
}

// test the fibonacci func implemented by task nested call but task will suspend in the middle state.
TEST_F(TaskTest, SubcallCase5)
{
    auto p = func7(10, vec);
    p.resume();
    ASSERT_EQ(vec[0], 10);
    // p.resume();
    // ASSERT_EQ(p.promise().result(), 55);
}
