#pragma once
#include "switcher.h"
#include <atomic>
#include <mutex>
#include <memory>
#include "timer.h"

// 类似posix的futex
// 阻塞等待一个目标值

namespace libgo
{

struct RutexWaiter : public LinkedNode
{
public:
    enum waiter_state {
        waiter_state_none,
        waiter_state_ready,
        waiter_state_interrupted,
        waiter_state_timeout,
    };

    explicit RutexWaiter(RoutineSwitcherI & sw) : switcher_(&sw) {}

    template<typename _Clock, typename _Duration>
    void sleep(const std::chrono::time_point<_Clock, _Duration> * abstime)
    {
        if (abstime) {
             RoutineSyncTimer::getInstance().schedule(timerId_, abstime,
                    [this] { this->wake_by_timer(); });
        }

        switcher_->sleep();
    }

    // 需要先加锁再执行
    bool wake(int state = waiter_state_ready) {
        if (waked_.load(std::memory_order_acquire))
            return true;

        state_.store(state, std::memory_order_relaxed);

        if (!switcher_->wake()) {
            return false;
        }

        waked_.store(true, std::memory_order_relaxed);
        return true;
    }

    void wake_by_timer() {
        std::unique_lock<std::mutex> lock(wakeMtx_, std::defer_lock);
        if (!lock.try_lock())
            return ;
        
        if (!safe_unlink())
            return ;

        if (wake(waiter_state_timeout))
            return ;

        // 可能还没来得及sleep, 往后等一等
        RoutineSyncTimer::getInstance().reschedule(timerId_,
                RoutineSyncTimer::now() + std::chrono::milliseconds(delayMs_));
        delayMs_ << 1;
    }

    inline bool safe_unlink();

    void join() {
        // ------- 1.避免不必要的wake调用, 加速wake结束
        waked_.store(true, std::memory_order_release);

        // ------- 2.先unlink
        // 有3种结果：
        // a) 成功: 说明没有进行中的notify.
        // b) 在锁上等待, 最终失败: notify已经unlink完成
        //   b-1) waitMtx::try_lock成功, 等待执行wake或已执行wake, 需要等待notify完成
        //   b-2) waitMtx::try_lock失败, 不会再执行wake, 无须等待notify完成
        // c) 没上锁，直接失败：同b
        safe_unlink();

        // ------- 3.等待wake完成, 并且锁死wakeMtx, 让后续的wake调用快速返回
        wakeMtx_.lock();
        // 代码执行到这里, notify函数都已经执行完毕

        if (timerId_) {
            // 3.等待timer结束 (防止timer那边刚好处在wake之前, 还持有this指针)
            RoutineSyncTimer::getInstance().join_unschedule(timerId_);
            // 代码执行到这里, timer函数已经执行完毕
        }
    }

    RoutineSwitcherI* switcher_;
    RoutineSyncTimer::TimerId timerId_ {};
    int delayMs_ = 1;
    std::atomic_int state_ {waiter_state_none};
    std::atomic_bool waked_ {false};
    std::mutex waitMtx_;
    std::atomic<Rutex*> owner_ {nullptr};
    std::atomic<std::mutex*> listMtx_ {nullptr};
};

struct Rutex
{
public:
    enum rutex_wait_return {
        rutex_wait_return_success = 0,
        rutex_wait_return_etimeout,
        rutex_wait_return_ewouldblock,
        rutex_wait_return_eintr,
    };

    Rutex() noexcept {}
    ~Rutex() noexcept {}

    Rutex(const Rutex&) = delete;
    Rutex& operator=(const Rutex&) = delete;

    rutex_wait_return wait(int32_t expectValue)
    {
        return wait_until(expectValue, RoutineSyncTimer::null_tp());
    }

    template<typename _Clock, typename _Duration>
    rutex_wait_return wait_until(int32_t expectValue,
            const std::chrono::time_point<_Clock, _Duration> * abstime)
    {
        if (value_.load(std::memory_order_relaxed) != expectValue) {
            std::atomic_thread_fence(std::memory_order_acquire);
            return rutex_wait_return_ewouldblock;
        }

        RutexWaiter rw(RoutineSyncPolicy::clsRef());

        {
            std::unique_lock<std::mutex> lock(mtx_);
            if (value_.load(std::memory_order_relaxed) != expectValue) {
                return rutex_wait_return_ewouldblock;
            }

            int state = rw.state_.load(std::memory_order_relaxed);
            switch (state) {
                case RutexWaiter::waiter_state_interrupted:
                    return rutex_wait_return_eintr;

                case RutexWaiter::waiter_state_ready:
                    return rutex_wait_return_success;
            }

            assert(!rw.safe_unlink());
            waiters_.push(&rw);
            rw.owner_.store(this, std::memory_order_relaxed);
        }

        rw.sleep(abstime);

        rw.join();

        int state = rw.state_.load(std::memory_order_relaxed);
        switch (state) {
            case RutexWaiter::waiter_state_none:
            case RutexWaiter::waiter_state_interrupted:
                return rutex_wait_return_eintr;

            case RutexWaiter::waiter_state_timeout:
                return rutex_wait_return_etimeout;
        }

        return rutex_wait_return_success;
    }

    int notify_one()
    {
        for (;;) {
            std::unique_lock<std::mutex> lock(mtx_);
            RutexWaiter *rw = static_cast<RutexWaiter *>(waiters_.front());
            if (!rw) {
                return 0;
            }

            // 先lock，后unlink, 和join里面的顺序形成ABBA, 确保join可以等到这个函数结束 
            std::unique_lock<std::mutex> lock(rw->waitMtx_, std::defer_lock);
            bool isLock = lock.try_lock();

            waiters_.unlink(rw);
            rw->owner_.store(nullptr, std::memory_order_relaxed);

            if (!islock) {
                continue;
            }

            lock.unlock();
            if (rw->wake()) {
                return 1;
            }
        }
    }

    int notify_all()
    {
        int n = 0;
        while (notify_one()) n++;
        return n;
    }

    int requeue(Rutex* other)
    {
        std::unique_lock<std::mutex> lock1(mtx_, std::defer_lock);
        std::unique_lock<std::mutex> lock2(other->mtx_, std::defer_lock);
        if (&mtx_ > &other->mtx_) {
            lock1.lock();
            lock2.lock();
        } else {
            lock2.lock();
            lock1.lock();
        }

        int n = 0;
        for (;;n++) {
            RutexWaiter *rw = static_cast<RutexWaiter *>(waiters_.front());
            if (!rw) {
                return n;
            }

            waiters_.unlink(rw);
            other->waiters_.push(rw);
            rw->owner_.store(&other, std::memory_order_relaxed);
        }
    }

    inline std::atomic<int>* value() { return &value_; }

private:
    friend struct RutexWaiter;

    std::atomic<int> value_ {0};
    LinkedList waiters_;
    std::mutex mtx_;
};

inline bool RutexWaiter::safe_unlink()
{
    Rutex* owner = nullptr;
    while ((owner = owner_.load(std::memory_order_acquire))) {
        std::unique_lock<std::mutex> lock(owner->mtx_);
        if (owner == owner_.load(std::memory_order_relaxed)) {
            owner->waiters_.unlink(this);
            owner_.store(nullptr, std::memory_order_relaxed);
            return true;
        }
    }

    return false;
}


} // namespace libgo
