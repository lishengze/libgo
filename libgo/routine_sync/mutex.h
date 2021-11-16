#pragma once
#include "rutex.h"

namespace libgo
{

// Implement bthread_mutex_t related functions
struct MutexInternal {
    butil::static_atomic<unsigned char> locked;
    butil::static_atomic<unsigned char> contended;
    unsigned short padding;
};

const MutexInternal c_mutex_contended_raw = {{1},{1},0};
const MutexInternal c_mutex_locked_raw = {{1},{0},0};

// Define as macros rather than constants which can't be put in read-only
// section and affected by initialization-order fiasco.
#define LIBGO_ROUTINE_SYNC_MUTEX_CONTENDED (*(const unsigned*)&::libgo::c_mutex_contended_raw)
#define LIBGO_ROUTINE_SYNC_MUTEX_LOCKED (*(const unsigned*)&::libgo::c_mutex_locked_raw)

static_assert(sizeof(unsigned) == sizeof(MutexInternal),
              "sizeof(MutexInternal) must equal sizeof(unsigned)");

struct Mutex
{
public:
    Mutex() noexcept {}
    ~Mutex() noexcept {}

    Mutex(const Mutex&) = delete;
    Mutex& operator=(const Mutex&) = delete;

    void lock()
    {
        lock(RoutineSyncTimer::null_tp());
    }

    bool try_lock()
    {
        MutexInternal* split = (MutexInternal*)rutex_.value();
        return 0 == split->locked.exchange(1, butil::memory_order_acquire);
    }

    bool is_lock()
    {
        MutexInternal* split = (MutexInternal*)rutex_.value();
        return split->locked == 1;
    }

    void unlock()
    {
        std::atomic<unsigned>* whole = (std::atomic<unsigned>*)rutex_.value();
        const unsigned prev = whole->exchange(0, butil::memory_order_release);
        if (prev == LIBGO_ROUTINE_SYNC_MUTEX_LOCKED) {
            return ;
        }

        rutex_.notify_one();
    }

    template<typename _Clock, typename _Duration>
    bool lock(const std::chrono::time_point<_Clock, _Duration> * abstime)
    {
        if (try_lock()) {
            return ;
        }

        return Rutex::rutex_wait_return_success == lock_contended(abstime);
    }

    template<typename _Clock, typename _Duration>
    inline int lock_contended(const std::chrono::time_point<_Clock, _Duration> * abstime) {
        std::atomic<unsigned>* whole = (std::atomic<unsigned>*)rutex_.value();
        while (whole->exchange(LIBGO_ROUTINE_SYNC_MUTEX_CONTENDED) & LIBGO_ROUTINE_SYNC_MUTEX_LOCKED) {
            int res = rutex_.wait_until(LIBGO_ROUTINE_SYNC_MUTEX_CONTENDED, abstime);
            if (res != Rutex::rutex_wait_return_success
                    && res != Rutex::rutex_wait_return_ewouldblock
                    && res != Rutex::rutex_wait_return_eintr)
                // 只剩timeout了
                return res;
        }
        return Rutex::rutex_wait_return_success;
    }

    Rutex* native() { return &rutex_; }

private:
    Rutex rutex_;
};

} // namespace libgo
