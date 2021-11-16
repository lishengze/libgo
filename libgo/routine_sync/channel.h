#pragma once
#include "condition_variable.h"
#include "timer.h"
#include <deque>
#include <exception>

namespace libgo
{

template <
    typename T,
    typename QueueT,
>
class ChannelImpl;

template <typename T>
class ChannelImplWithSignal
{
public:
    template<typename _Clock, typename _Duration>
    bool pop_impl_with_signal_noqueued(T & t, bool isWait,
            const std::chrono::time_point<_Clock, _Duration>* abstime,
            std::unique_lock<Mutex> & lock)
    {
        if (closed_)
            return false;

        if (pushQ_ && waiting_) {   // 有push协程在等待, 读出来 & 清理
            t = *pushQ_;
            pushQ_ = nullptr;
            waiting_->fast_notify_all(lock);
            waiting_ = nullptr;
            return true;
        }

        if (!isWait)
            return false;

        // 开始等待
        T temp;
        ConditionVariable waiting;

        popQ_ = &temp;
        waiting_ = &waiting;

        (void)waiting.wait_until_p(lock, abstime, []{ return true; });
        if (popQ_ != &temp) {
            // 完成, 此时不必判断status
            t = std::move(temp);    // 对外部T的写操作放到本线程来做, 降低使用难度
            return true;
        }

        // 超时, 清理
        popQ_ = nullptr;
        waiting_ = nullptr;
        return false;
    }

    template<typename _Clock, typename _Duration>
    bool pop_impl_with_signal(T & t, bool isWait,
            const std::chrono::time_point<_Clock, _Duration>* abstime)
    {
        std::unique_lock<Mutex> lock(mtx_);
        if (!popQ_) {
            return pop_impl_with_signal_noqueued(t, isWait, abstime, lock);
        }

        if (!isWait) {
            return false;
        }

        // 有其他pop在等待, 进pop队列
        auto p = [this]{ return !popQ_; };
        std::cv_status status = popCv_.wait_until_p(lock, *abstime, p);
        if (std::cv_status::timeout == status)
            return false;

        return pop_impl_with_signal_noqueued(t, isWait, abstime, lock);
    }

    template<typename _Clock, typename _Duration>
    bool push_impl_with_signal_noqueued(T const& t, bool isWait,
            const std::chrono::time_point<_Clock, _Duration>* abstime,
            std::unique_lock<Mutex> & lock)
    {
        if (closed_)
            return false;

        if (popQ_ && waiting_) {   // 有pop协程在等待, 写入 & 清理
            *popQ_ = t;
            popQ_ = nullptr;
            waiting_->fast_notify_all(lock);
            waiting_ = nullptr;
            return true;
        }

        if (!isWait)
            return false;

        // 开始等待
        ConditionVariable waiting;

        pushQ_ = &t;
        waiting_ = &waiting;

        (void)waiting.wait_until_p(lock, abstime, []{ return true; });
        if (pushQ_ != &t) {
            // 完成, 此时不必判断status
            return true;
        }

        // 超时, 清理
        pushQ_ = nullptr;
        waiting_ = nullptr;
        return false;
    }

    template<typename _Clock, typename _Duration>
    bool push_impl_with_signal(T const& t, bool isWait,
            const std::chrono::time_point<_Clock, _Duration>* abstime)
    {
        std::unique_lock<Mutex> lock(mtx_);
        if (!pushQ_) {
            return push_impl_with_signal_noqueued(t, isWait, abstime, lock);
        }

        if (!isWait) {
            return false;
        }

        // 有其他push在等待, 进push队列
        auto p = [this]{ return !pushQ_; };
        std::cv_status status = pushCv_.wait_until_p(lock, *abstime, p);
        if (std::cv_status::timeout == status)
            return false;

        return push_impl_with_signal_noqueued(t, isWait, abstime, lock);
    }

protected:
    Mutex mtx_;
    T const* pushQ_ {nullptr};
    T* popQ_ {nullptr};
    ConditionVariable* waiting_ {nullptr};
    ConditionVariable pushCv_;
    ConditionVariable popCv_;
    bool closed_ {false};
};

template <
    typename T,
    typename QueueT,
>
class ChannelImpl : public ChannelImplWithSignal<T>
{
public:
    explicit ChannelImpl(std::size_t capacity = 0)
        : cap_(capacity)
    {
    }

    template<typename _Clock, typename _Duration>
    bool push(T const& t, bool isWait,
            const std::chrono::time_point<_Clock, _Duration>* abstime = nullptr)
    {
        if (cap_) {
            return push_impl_with_cap(t, isWait, abstime);
        }

        return push_impl_with_signal(t, isWait, abstime);
    }

    template<typename _Clock, typename _Duration>
    bool pop(T & t, bool isWait,
            const std::chrono::time_point<_Clock, _Duration>* abstime = nullptr)
    {
        if (cap_) {
            return pop_impl_with_cap(t, isWait, abstime);
        }

        return pop_impl_with_signal(t, isWait, abstime);
    }

    std::size_t capacity() const
    {
        return cap_;
    }

    bool closed() const
    {
        return closed_;
    }

    std::size_t size()
    {
        std::unique_lock<Mutex> lock(mtx_);
        return q_.size();
    }

    std::size_t empty()
    {
        return !size();
    }

    void close()
    {
        std::unique_lock<Mutex> lock(mtx_);
        closed_ = true;
        if (!cap_) {
            pushQ_ = nullptr;
            popQ_ = nullptr;
            if (waiting_) {
                waiting_->fast_notify_all(lock);
                waiting_ = nullptr;
            }
        }

        pushCv_.fast_notify_all(lock);
        popCv_.fast_notify_all(lock);
        QueueT q;
        std::swap(q, q_);
    }

private:
    template<typename _Clock, typename _Duration>
    bool push_impl_with_cap(T const& t, bool isWait,
            const std::chrono::time_point<_Clock, _Duration>* abstime)
    {
        std::unique_lock<Mutex> lock(mtx_);
        if (closed_)
            return false;

        if (q_.size() < cap_) {
            q_.push_back(t);
            popCv_.noitfy_one();
            return true;
        }

        if (!isWait) {
            return false;
        }

        auto p = [this]{ return q_.size() < cap_; };

        std::cv_status status = pushCv_.wait_until_p(lock, abstime, p);

        if (status == std::cv_status::timeout)
            return false;

        if (closed_)
            return false;

        q_.push_back(t);
        popCv_.noitfy_one();
        return true;
    }

    template<typename _Clock, typename _Duration>
    bool pop_impl_with_cap(T & t, bool isWait,
            const std::chrono::time_point<_Clock, _Duration>* abstime)
    {
        std::unique_lock<Mutex> lock(mtx_);
        if (closed_)
            return false;

        if (!q_.empty()) {
            t = q_.front();
            q_.pop();
            pushCv_.noitfy_one();
            return true;
        }

        if (!isWait) {
            return false;
        }

        auto p = [this]{ return !q_.empty(); };

        std::cv_status status = popCv_.wait_until_p(lock, abstime, p);

        if (status == std::cv_status::timeout)
            return false;

        if (closed_)
            return false;

        t = q_.front();
        q_.pop();
        pushCv_.noitfy_one();
        return true;
    }

private:
    QueueT q_;
    std::size_t cap_;
};

// 仅计数
template <
    typename QueueT
>
class ChannelImpl<nullptr_t> : public ChannelImplWithSignal<nullptr_t>
{
public:
    explicit ChannelImpl(std::size_t capacity = 0)
        : cap_(capacity), count_(0)
    {
    }

    template<typename _Clock, typename _Duration>
    bool push(nullptr_t const& t, bool isWait,
            const std::chrono::time_point<_Clock, _Duration>* abstime = nullptr)
    {
        if (cap_) {
            return push_impl_with_cap(t, isWait, abstime);
        }

        return push_impl_with_signal(t, isWait, abstime);
    }

    template<typename _Clock, typename _Duration>
    bool pop(nullptr_t & t, bool isWait,
            const std::chrono::time_point<_Clock, _Duration>* abstime = nullptr)
    {
        if (cap_) {
            return pop_impl_with_cap(t, isWait, abstime);
        }

        return pop_impl_with_signal(t, isWait, abstime);
    }

    std::size_t capacity() const
    {
        return cap_;
    }

    bool closed() const
    {
        return closed_;
    }

    std::size_t size()
    {
        std::unique_lock<Mutex> lock(mtx_);
        return count_;
    }

    std::size_t empty()
    {
        return !size();
    }

    void close()
    {
        std::unique_lock<Mutex> lock(mtx_);
        closed_ = true;
        if (!cap_) {
            pushQ_ = nullptr;
            popQ_ = nullptr;
            if (waiting_) {
                waiting_->fast_notify_all(lock);
                waiting_ = nullptr;
            }
        }

        pushCv_.fast_notify_all(lock);
        popCv_.fast_notify_all(lock);
        count_ = 0;
    }

private:
    template<typename _Clock, typename _Duration>
    bool push_impl_with_cap(nullptr_t const& t, bool isWait,
            const std::chrono::time_point<_Clock, _Duration>* abstime)
    {
        std::unique_lock<Mutex> lock(mtx_);
        if (closed_)
            return false;

        if (count_ < cap_) {
            ++count_;
            popCv_.noitfy_one();
            return true;
        }

        if (!isWait) {
            return false;
        }

        auto p = [this]{ return count_ < cap_; };

        std::cv_status status = pushCv_.wait_until_p(lock, abstime, p);

        if (status == std::cv_status::timeout)
            return false;

        if (closed_)
            return false;

        ++count_;
        popCv_.noitfy_one();
        return true;
    }

    template<typename _Clock, typename _Duration>
    bool pop_impl_with_cap(nullptr_t & t, bool isWait,
            const std::chrono::time_point<_Clock, _Duration>* abstime)
    {
        std::unique_lock<Mutex> lock(mtx_);
        if (closed_)
            return false;

        if (count_ > 0) {
            --count_;
            pushCv_.noitfy_one();
            return true;
        }

        if (!isWait) {
            return false;
        }

        auto p = [this]{ return count_ > 0; };

        std::cv_status status = popCv_.wait_until_p(lock, abstime, p);

        if (status == std::cv_status::timeout)
            return false;

        if (closed_)
            return false;

        --count_;
        pushCv_.noitfy_one();
        return true;
    }

private:
    std::size_t cap_;
    std::size_t count_;
};

template <
    typename T,
    template<class U, class Alloc> class Queue = std::deque<U, Alloc>
>
class Channel
{
    typedef ChannelImpl<T, Queue<T>> ImplType;
    typedef std::shared_ptr<ImplType> impl_;

public:
    explicit Channel(std::size_t capacity = 0,
            bool throw_ex_if_operator_failed = true)
    {
        impl_ = std::make_shared<ImplType>(capacity);
        throw_ex_if_operator_failed_ = throw_ex_if_operator_failed;
    }

    Channel const& operator<<(T const& t) const
    {
        if (!impl_->push(t, true) && throw_ex_if_operator_failed_) {
            throw std::runtime_error("channel operator<<(T) error");
        }
        return *this;
    }

    Channel const& operator>>(T & t) const
    {
        if (!impl_->pop(t, true) && throw_ex_if_operator_failed_) {
            throw std::runtime_error("channel operator>>(T) error");
        }
        return *this;
    }

    Channel const& operator>>(std::nullptr_t ignore) const
    {
        T t;
        if (!impl_->pop(t, true) && throw_ex_if_operator_failed_) {
            throw std::runtime_error("channel operator<<(ignore) error");
        }
        return *this;
    }

    bool Push(T const& t) const
    {
        return impl_->push(t, true);
    }

    bool Pop(T & t) const
    {
        return impl_->pop(t, true);
    }

    bool Pop(std::nullptr_t ignore) const
    {
        T t;
        return impl_->pop(t, true);
    }

    bool TryPush(T const& t) const
    {
        return impl_->push(t, false);
    }

    bool TryPop(T & t) const
    {
        return impl_->pop(t, false);
    }

    bool TryPop(std::nullptr_t ignore) const
    {
        T t;
        return impl_->pop(t, false);
    }

    template <typename Rep, typename Period>
    bool TimedPush(T const& t, std::chrono::duration<Rep, Period> dur) const
    {
        auto abstime = RoutineSyncTimer::now() + dur;
        return impl_->push(t, true, &abstime);
    }

    bool TimedPush(T const& t, RoutineSyncTimer::clock_type::time_point deadline) const
    {
        return impl_->push(t, true, &deadline);
    }

    template <typename Rep, typename Period>
    bool TimedPop(T & t, std::chrono::duration<Rep, Period> dur) const
    {
        auto abstime = RoutineSyncTimer::now() + dur;
        return impl_->pop(t, true, &abstime);
    }

    bool TimedPop(T & t, RoutineSyncTimer::clock_type::time_point deadline) const
    {
        return impl_->pop(t, true, &deadline);
    }

    template <typename Rep, typename Period>
    bool TimedPop(std::nullptr_t ignore, std::chrono::duration<Rep, Period> dur) const
    {
        auto abstime = RoutineSyncTimer::now() + dur;
        T t;
        return impl_->pop(t, true, &abstime);
    }

    bool TimedPop(std::nullptr_t ignore, RoutineSyncTimer::clock_type::time_point deadline) const
    {
        T t;
        return impl_->pop(t, true, &deadline);
    }

    bool Unique() const
    {
        return impl_.unique();
    }

    void Close() const {
        impl_->close();
    }

    inline bool Closed() const {
        return impl_->closed();
    }

    // ------------- 兼容旧版接口
    bool empty() const
    {
        return impl_->empty();
    }

    std::size_t size() const
    {
        return impl_->size();
    }

private:
    bool throw_ex_if_operator_failed_;
};


template <>
class Channel<void> : public Channel<std::nullptr_t>
{
public:
    explicit Channel(std::size_t capacity = 0)
        : Channel<std::nullptr_t>(capacity)
    {}
};

//template <typename T>
//using co_chan = Channel<T>;

} //namespace libgo
