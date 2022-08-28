#include "test_simple.h"
#include "coroutine.h"
#include "tool.h"


void go_func0 () 
{
    while(true)
    {
        // co_sleep(3000);

        std::this_thread::sleep_for(std::chrono::seconds(2));

        cout << "go_func0: " << std::this_thread::get_id()  << ", " << NanoTimeStr() <<  endl;
    }
    
}

void go_func1 () 
{
    cout << "go_func1: " << std::this_thread::get_id()  << " start!" << endl;
    while(true)
    {
        // co_sleep(3000);
        // std::this_thread::sleep_for(std::chrono::seconds(3));
        // go go_func0;
        // cout << "go_func1: " << std::this_thread::get_id()  << ", " << NanoTimeStr() <<  endl;
        // int a = 10;
    }

    cout << "go_func1: " << std::this_thread::get_id()  << " end!" << endl;


    // std::this_thread::sleep_for(std::chrono::seconds(10));
    // go go_func0;
    // cout << "go_func1: " << std::this_thread::get_id()  << ", " << NanoTimeStr() <<  endl;    
}

void go_func2() 
{
    while(true)
    {
        // co_sleep(3000);

        std::this_thread::sleep_for(std::chrono::seconds(3));

        cout << "go_func2: " << std::this_thread::get_id()  << ", " << NanoTimeStr() <<  endl;
    }
}

void go_func3() 
{
    while(true)
    {
        // co_sleep(3000);

        std::this_thread::sleep_for(std::chrono::seconds(3));

        cout << "go_func3: " << std::this_thread::get_id()  << ", " << NanoTimeStr() <<  endl;
    }
}

void test1() { 

    cout << "test1.main_thread:  " << std::this_thread::get_id() << endl;

    go go_func1;
    go go_func2;

    // std::thread sched_thread = std::thread([](){ co_sched.Start(0, 1024);});
    // sched_thread.detach();

    co_sched.Start();
    cout << "test1 over" << endl;
}

void go_func(int i ) {
    cout << "go_func1: " << std::this_thread::get_id()  << ", " << NanoTimeStr() <<  endl;
}

void test_thread() {
    int coroutine_count = 2;
    for (int i = 0; i < coroutine_count; ++i) {
        go std::bind(go_func, i);
    }

    co_sched.Start();

    cout << "test_thread over" << endl;    
}

void thread_main() {
    go go_func3;
}

void test_multi_thread() {
    cout << "test_multi_thread main_thread: " << std::this_thread::get_id() << endl;
    std::thread sched_thread = std::thread([](){ 
        cout << "Add go_func3 thread: " << std::this_thread::get_id()  << endl;
        go go_func3;
    });

    go go_func2;

    co_sched.Start();

    if (sched_thread.joinable()) {
        sched_thread.join();
    }
}


void test_thread_block() {
    cout << "test_thread_block main_thread: " << std::this_thread::get_id() << endl;
    for (int i = 0; i < 2; ++i) {
        go go_func1;
    }
    go go_func2;

    co_sched.Start(4);
}

void TestSimple()
{
    // test1();
    // test_thread();
    // test_multi_thread();

    test_thread_block();
}