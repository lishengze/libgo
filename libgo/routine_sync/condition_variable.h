#pragma once
#include "Mutex.h"
#include <condition_variable>
#include <mutex>

namespace libgo
{

struct ConditionVariable
{
public:
    ConditionVariable() noexcept;
    ~ConditionVariable() noexcept;

    ConditionVariable(const ConditionVariable&) = delete;
    ConditionVariable& operator=(const ConditionVariable&) = delete;

    void notify_one()
    {
        rutex_.value()->fetch_add(1, std::memory_order_release);
        rutex_.notify_one();
    }

    void notify_all()
    {
        rutex_.value()->fetch_add(1, std::memory_order_release);
        rutex_.notify_all();
    }

    // 快速notify_all, 要求ConditionVariable的每个wait和notify都只搭配同一把锁使用
    void fast_notify_all(std::unique_lock<Mutex>& lock)
    {
        rutex_.value()->fetch_add(1, std::memory_order_release);
        rutex_.requeue(lock.mutex()->native());
    }

    void wait(std::unique_lock<Mutex>& lock)
    {
        wait_until_impl(lock, RoutineSyncTimer::null_tp());
    }

    template<typename Predicate>
    void wait(std::unique_lock<Mutex>& lock, Predicate p)
    {
        while (!p())
            wait(lock);
    }

    template<typename _Clock, typename _Duration>
    std::cv_status wait_until(std::unique_lock<Mutex>& lock,
            const std::chrono::time_point<_Clock, _Duration>& abstime)
    {
        return wait_until_impl(lock, &abstime);
    }

    template<typename _Clock, typename _Duration, typename Predicate>
    bool wait_until(std::unique_lock<Mutex>& lock,
        const std::chrono::time_point<_Clock, _Duration>& abstime,
        Predicate p)
    {
        while (!p())
            if (wait_until(lock, abstime) == std::cv_status::timeout)
                return p();

        return true;
    }

    template<typename _Rep, typename _Period>
    std::cv_status wait_for(std::unique_lock<Mutex>& lock,
        const std::chrono::duration<_Rep, _Period>& dur)
    {
        return wait_until(lock, RoutineSyncTimer::now() + dur);
    }

    template<typename _Rep, typename _Period, typename Predicate>
    bool wait_for(std::unique_lock<Mutex>& lock,
            const std::chrono::duration<_Rep, _Period>& dur,
            Predicate p)
    {
        return wait_until(lock, RoutineSyncTimer::now() + dur, std::move(p));
    }

    template<typename _Clock, typename _Duration>
    std::cv_status wait_until_p(std::unique_lock<Mutex>& lock,
            const std::chrono::time_point<_Clock, _Duration> * abstime,
            Predicate p)
    {
        std::cv_status status = std::cv_status::no_timeout;
        if (abstime) {
            status = wait_until(lock, *abstime, p);
        } else {
            wait(lock, p);
        }
        return status;
    }

private:
    template<typename _Clock, typename _Duration>
    std::cv_status wait_until_impl(std::unique_lock<Mutex>& lock,
            const std::chrono::time_point<_Clock, _Duration> * abstime)
    {
        const int32_t expectedValue = rutex_.value()->load(std::memory_order_relaxed);
        lock.mutex()->unlock();

        std::cv_status res = (
                Rutex::rutex_wait_return_etimeout == rutex_.wait_until(expectValue, abstime))
            ? std::cv_status::timeout : std::cv_status::no_timeout;

        lock.mutex()->lock_contended(RoutineSyncTimer::null_tp());
        return res;
    }

private:
    Rutex rutex_;
};

} // namespace libgo
