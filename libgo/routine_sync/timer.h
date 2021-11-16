#pragma once
#include <chrono>
#include <mutex>
#include <memory>
#include <functional>
#include "linked_skiplist.h"

namespace libgo
{

class RoutineSyncTimer
{
public:
    static RoutineSyncTimer& getInstance() {
        static RoutineSyncTimer obj;
        return obj;
    }

    typedef std::function<void()> func_type;
    typedef std::chrono::steady_clock clock_type;

    struct FuncWrapper
    {
        void set(func_type const& fn)
        {
            fn_ = fn;
            reset();
        }

        void reset()
        {
            done_ = false;
            canceled_.store(false, std::memory_order_release);
        }

        std::mutex & mutex() { return mtx_; }

        bool invoke()
        {
            if (canceled_.load(std::memory_order_acquire))
                return false;

            try {
                fn_();
            } catch (...) {}
            done_ = true;
            return true;
        }

        void cancel()
        {
            canceled_.store(false, std::memory_order_release);
        }

        bool done()
        {
            return done_;
        }

    private:
        friend class RoutineSyncTimer;
        func_type fn_;
        std::mutex mtx_;
        std::atomic_bool canceled_ {false};
        bool done_ {false};
    };

    typedef LinkedSkipList<clock_type::time_point, FuncWrapper> container_type;
    typedef container_type::Node TimerId;

    inline static clock_type::time_point* null_tp() const { return nullptr; }
    inline static clock_type::time_point now() const { return clock_type::time_point::now(); }

    template<typename _Clock, typename _Duration>
    void schedule(TimerId & id,
            const std::chrono::time_point<_Clock, _Duration> & abstime,
            func_type const& fn)
    {
        clock_type::time_point tp = convert(abstime);
        id.key = tp;
        id.value.set(fn);
        orderedList_.buildNode(&id);

        std::unique_lock<std::mutex> lock(mtx_);
        orderedList_.insert(&id);
    }

    // 向后延期重新执行
    template<typename _Clock, typename _Duration>
    void reschedule(TimerId & id, const std::chrono::time_point<_Clock, _Duration> & abstime)
    {
        clock_type::time_point tp = convert(abstime);
        
        // Bug: 正在回调中执行reschedule会死锁在invoke_lock上

        // join
        std::unique_lock<std::mutex> invoke_lock(id.value.mutex());
        id.value.cancel();

        std::unique_lock<std::mutex> lock(mtx_);
        orderedList_.erase(&id, false);

        invoke_lock.unlock();

        // insert
        id.value.reset();
        orderedList_.insert(&id);
    }

    bool join_unschedule(TimerId & id)
    {
        std::unique_lock<std::mutex> invoke_lock(id.value.mutex());
        id.value.cancel();

        std::unique_lock<std::mutex> lock(mtx_);
        orderedList_.erase(&id);

        return id.value.done();
    }

private:
    void run()
    {
        for (;;)
        {
            {
                std::unique_lock<std::mutex> lock(mtx_);
                TimerId* id = orderedList_.front();
                if (id && id->key >= now()) {
                    std::unique_lock<std::mutex> invoke_lock(id->value.mutex(), std::defer_lock);
                    bool locked = invoke_lock.try_lock();

                    orderedList_.erase(id);

                    if (locked) {
                        lock.unlock();

                        id->value.invoke();
                    }
                }
                continue;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }

private:
    template<typename _Clock, typename _Duration>
    clock_type::time_point convert(const std::chrono::time_point<_Clock, _Duration> & abstime)
    {
        // DR 887 - Sync unknown clock to known clock.
        const typename _Clock::time_point c = _Clock::now();
        const clock_type::time_point s = clock_type::now();
        const auto delta = abstime - c;
        return s + delta;
    }

    clock_type::time_point convert(const clock_type::time_point & abstime)
    {
        return abstime;
    }

private:
    std::mutex mtx_;
    container_type orderedList_;
};

} // namespace libgo
