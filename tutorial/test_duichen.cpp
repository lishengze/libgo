#include "coroutine.h"
#include <iostream>
#include <thread>
#include <string>
#include <chrono>
#include <time.h>
using namespace std;

const long MILLISECONDS_PER_SECOND = 1000;
const long MICROSECONDS_PER_MILLISECOND = 1000;
const long NANOSECONDS_PER_MICROSECOND = 1000;

const long MICROSECONDS_PER_SECOND =
        MICROSECONDS_PER_MILLISECOND * MILLISECONDS_PER_SECOND;
const long NANOSECONDS_PER_MILLISECOND =
        NANOSECONDS_PER_MICROSECOND * MICROSECONDS_PER_MILLISECOND;
const long NANOSECONDS_PER_SECOND =
        NANOSECONDS_PER_MILLISECOND * MILLISECONDS_PER_SECOND;


inline long NanoTime() {
    std::chrono::high_resolution_clock::time_point curtime = std::chrono::high_resolution_clock().now();
    long orin_nanosecs = std::chrono::duration_cast<std::chrono::nanoseconds>(curtime.time_since_epoch()).count();
    return orin_nanosecs;
//    return NanoTimer::getInstance()->getNano();
}

inline std::string ToSecondStr(long nano, const char* format="%Y-%m-%d %H:%M:%S") {
    if (nano <= 0)
        return std::string("NULL");
    nano /= NANOSECONDS_PER_SECOND;
    struct tm* dt = {0};
    char buffer[30];
    dt = gmtime(&nano);
    strftime(buffer, sizeof(buffer), format, dt);
    return std::string(buffer);
}


/*

*/
inline std::string NanoTimeStr() {
    long nano_time = NanoTime();
    string time_now = ToSecondStr(nano_time, "%Y-%m-%d %H:%M:%S");
    time_now += "." + std::to_string(nano_time % NANOSECONDS_PER_SECOND);
    return time_now;
}

void go_func0 () 
{
    while(true)
    {
        // co_sleep(3000);

        std::this_thread::sleep_for(std::chrono::seconds(2));

        printf("go_func0: %s \n", NanoTimeStr().c_str());
    }
    
}

void go_func1 () 
{
    // while(true)
    // {
    //     // co_sleep(3000);

    //     std::this_thread::sleep_for(std::chrono::seconds(3));

    //     go go_func0;

    //     cout << "go_func1: " << std::this_thread::get_id()  << ", " << NanoTimeStr() <<  endl;
    // }


        std::this_thread::sleep_for(std::chrono::seconds(3));

        go go_func0;

        printf("go_func1: %s \n", NanoTimeStr().c_str());
}

void go_func2() 
{
    while(true)
    {
        // co_sleep(3000);

        std::this_thread::sleep_for(std::chrono::seconds(3));

        printf("go_func3: %s \n", NanoTimeStr().c_str());
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

int main()
{
    test1();
    // test_thread();
}