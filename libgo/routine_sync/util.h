#pragma once
#include <memory>
#include <atomic>

namespace libgo
{

// 默认不做delete的引用计数基类
template <typename T>
struct RefBase
{
public:
    void inc() {
        ++ref_;
    }

    void dec() {
        if (0 == --ref_) {
            if (deleter_)
                deleter_(dynamic_cast<T*>(this));
        }
    }

    typedef std::function<void(T*)> DeleterFunc;

    void setDeleter(DeleterFunc deleter) {
        deleter_ = deleter;
    }

private:
    std::atomic_long ref_ {1};
    DeleterFunc deleter_;
};

template <typename T>
struct RefBaseGuard
{
public:
    explicit RefBaseGuard(T* obj) : obj_(obj) {
        obj_->inc();
    }

    ~RefBaseGuard() {
        obj_->dec();
    }

private:
    T* obj_;
};

} // namespace libgo
