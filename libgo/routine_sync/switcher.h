#pragma once
#include <memory>
#include <functional>
#include <condition_variable>
#include <mutex>
#include <chrono>
#include "linked_list.h"
#include "util.h"

namespace libgo
{

// routine切换器
// routine_sync套件需要针对每一种routine自定义一个switcher类。
struct RoutineSwitcherI
{
public:
    virtual ~RoutineSwitcherI() {}

    // 在routine中调用的接口，用于休眠当前routine
    virtual void sleep() = 0;

    // 在其他routine中调用，用于唤醒休眠的routine
    // @return: 返回唤醒成功or失败
    // @要求: 一次sleep多次wake，只有其中一次wake成功，并且其他wake不会产生副作用
    virtual bool wake() = 0;

    // 判断是否在协程中 (子类switcher必须实现这个接口)
    //static bool isInRoutine();

    // 返回协程私有变量的switcher (子类switcher必须实现这个接口)
    //static RoutineSwitcherI & clsRef()
};

struct PThreadSwitcher : public RoutineSwitcherI
{
public:
    virtual void sleep() override
    {
        std::unique_lock<std::mutex> lock(mtx_);
        waiting_ = true;
        while (waiting_)
            cond_.wait(lock);
    }

    virtual bool wake() override
    {
        std::unique_lock<std::mutex> lock(mtx_);
        if (!waiting_)
            return false;
        waiting_ = false;
        cond_.notify_one();
        return true;
    }

    static bool isInRoutine() { return true; }

    static RoutineSwitcherI & clsRef()
    {
        static thread_local PThreadSwitcher pts;
        return &pts;
    }

private:
    std::mutex mtx_;
    std::condition_variable cond_;
    bool waiting_ = false;
};

// 配置器
struct RoutineSyncPolicy
{
public:
    // 注册switchers
    template <typename ... Switchers>
    static void registerSwitchers() {
        clsRefFunction() = &RoutineSyncPolicy::clsRef<Switchers...>;
    }

    static RoutineSwitcherI& clsRef()
    {
        return clsRefFunction();
    }

private:
    typedef std::function<RoutineSwitcherI& ()> ClsRefFunction;

    static ClsRefFunction & clsRefFunction() {
        static ClsRefFunction fn;
        return fn;
    }

    template <typename S1, typename ... Switchers>
    inline static RoutineSwitcherI& clsRef() {
        if (S1::isInRoutine()) {
            return S1::clsRef();
        }

        return clsRef<Switchers...>()();
    }

    template <typename S1>
    inline static RoutineSwitcherI& clsRef() {
        if (S1::isInRoutine()) {
            return S1::clsRef();
        }

        return PThreadSwitcher::clsRef();
    }
};

} // namespace libgo
