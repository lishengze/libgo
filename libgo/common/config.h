#pragma once
#include "cmake_config.h"
#include <unordered_map>
#include <list>
#include <errno.h>
#include <string.h>
#include <cstdlib>
#include <stdio.h>
#include <atomic>
#include <mutex>
#include <assert.h>
#include <deque>
#include <string>
#include <type_traits>
#include <stddef.h>
#include <exception>
#include <vector>
#include <set>
#include <map>
#include <functional>
#include <chrono>
#include <memory>
#include <queue>
#include <algorithm>

#define LIBGO_DEBUG 0

#if defined(__APPLE__) || defined(__FreeBSD__)
# define LIBGO_SYS_FreeBSD 1
# define LIBGO_SYS_Unix 1
#elif defined(__linux__)
# define LIBGO_SYS_Linux 1
# define LIBGO_SYS_Unix 1
#elif defined(_WIN32)
# define LIBGO_SYS_Windows 1
#endif

// VS2013��֧��thread_local
#if defined(_MSC_VER) && _MSC_VER < 1900
# define thread_local __declspec(thread)
#endif

#if defined(__GNUC__) && (__GNUC__ > 3 ||(__GNUC__ == 3 && __GNUC_MINOR__ >= 1))
# define ALWAYS_INLINE __attribute__ ((always_inline)) inline 
#else
# define ALWAYS_INLINE inline
#endif

#if defined(LIBGO_SYS_Unix)
# define LIKELY(x) __builtin_expect(!!(x), 1)
# define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
# define LIKELY(x) x
# define UNLIKELY(x) x
#endif

#if defined(LIBGO_SYS_Linux)
# define ATTRIBUTE_WEAK __attribute__((weak))
#elif defined(LIBGO_SYS_FreeBSD)
# define ATTRIBUTE_WEAK __attribute__((weak_import))
#endif

#if defined(LIBGO_SYS_Windows)
#pragma warning(disable : 4996)
#endif

#if defined(LIBGO_SYS_Windows)
# define FCONTEXT_CALL __stdcall
#else
# define FCONTEXT_CALL
#endif

#if defined(LIBGO_SYS_Unix)
#include <unistd.h>
#include <sys/types.h>
#endif

#if defined(LIBGO_SYS_Windows)
#include <Winsock2.h>
#include <Windows.h>
#endif

namespace co
{

void LibgoInitialize();

template <typename T>
using atomic_t = std::atomic<T>;

///---- debugger flags
static const uint64_t dbg_none              = 0;
static const uint64_t dbg_all               = ~(uint64_t)0;
static const uint64_t dbg_hook              = 0x1;
static const uint64_t dbg_yield             = 0x1 << 1;
static const uint64_t dbg_scheduler         = 0x1 << 2;
static const uint64_t dbg_task              = 0x1 << 3;
static const uint64_t dbg_switch            = 0x1 << 4;
static const uint64_t dbg_ioblock           = 0x1 << 5;
static const uint64_t dbg_suspend           = 0x1 << 6;
static const uint64_t dbg_exception         = 0x1 << 7;
static const uint64_t dbg_syncblock         = 0x1 << 8;
static const uint64_t dbg_timer             = 0x1 << 9;
static const uint64_t dbg_scheduler_sleep   = 0x1 << 10;
static const uint64_t dbg_sleepblock        = 0x1 << 11;
static const uint64_t dbg_spinlock          = 0x1 << 12;
static const uint64_t dbg_fd_ctx            = 0x1 << 13;
static const uint64_t dbg_debugger          = 0x1 << 14;
static const uint64_t dbg_signal            = 0x1 << 15;
static const uint64_t dbg_channel           = 0x1 << 16;
static const uint64_t dbg_thread            = 0x1 << 17;
static const uint64_t dbg_sys_max           = dbg_debugger;
///-------------------

// Э�����׳�δ�����쳣ʱ�Ĵ�����ʽ
enum class eCoExHandle : uint8_t
{
    immedaitely_throw,  // �����׳�
    on_listener,        // ʹ��listener����, ���û����listener�������׳�
};

typedef void*(*stack_malloc_fn_t)(size_t size);
typedef void(*stack_free_fn_t)(void *ptr);

///---- ����ѡ��
struct CoroutineOptions
{
    /*********************** Debug options **********************/
    // ����ѡ��, ����: dbg_switch �� dbg_hook|dbg_task|dbg_wait
    uint64_t debug = 0;            

    // ������Ϣ���λ�ã���д�������������ض������λ��
    FILE* debug_output = stdout;   
    /************************************************************/

    /**************** Stack and Exception options ***************/
    // Э�����׳�δ�����쳣ʱ�Ĵ�����ʽ
    eCoExHandle exception_handle = eCoExHandle::immedaitely_throw;

    // Э��ջ��С����, ֻ��Ӱ���ڴ�ֵ����֮���´�����P, �������״�Runǰ����.
    // stack_size�������ò�����1MB
    // Linuxϵͳ��, ����2MB��stack_size�ᵼ���ύ�ڴ��ʹ������1MB��stack_size��10��.
    uint32_t stack_size = 1 * 1024 * 1024; 
    /************************************************************/

    // epollÿ�δ�����event����(Windows����Ч)
    uint32_t epoll_event_size = 10240;

    // �Ƿ�����Э��ͳ�ƹ���(����һ���������, Ĭ�ϲ�����)
    bool enable_coro_stat = false;

    // ��Э��ִ�г�ʱʱ��(��λ��΢��) (����ʱ����ǿ��stealʣ������, �ɷ��������߳�)
    uint32_t cycle_timeout_us = 100 * 1000; 

    // �����̵߳Ĵ���Ƶ��(��λ��΢��)
    uint32_t dispatcher_thread_cycle_us = 1000; 
    //  ���ؾ��ⴥ���ı���,ȡֵ��Χ 0 - 1
    //  ��ĳ��ִ������Э��������ƽ��ֵ��load_balance_rate�ͻᴥ�����ؾ���
    //  ��ִ�е�Э������Ƚ���ʱ,��ֵ�������һ��,Э������Ƚ���ʱ,�������һ��
    float load_balance_rate = 0.01; 

    // ջ�����ñ����ڴ�ε��ڴ�ҳ����(��linux����Ч)(Ĭ��Ϊ0, ��:������)
    // ��ջ���ڴ������ǰ��ҳ����Ϊprotect����.
    // ���Կ�����ѡ��ʱ, stack_size��������protect_stack_page+1ҳ
    int & protect_stack_page;

    // ����ջ�ڴ����(malloc/free)
    // ʹ��fiber��Э�̵ײ�ʱ��Ч
    stack_malloc_fn_t & stack_malloc_fn;
    stack_free_fn_t & stack_free_fn;

    CoroutineOptions();

    ALWAYS_INLINE static CoroutineOptions& getInstance()
    {
        static CoroutineOptions obj;
        return obj;
    }
};

int GetCurrentProcessID();
int GetCurrentThreadID();
int GetCurrentCoroID();
std::string GetCurrentTimeStr();
const char* BaseFile(const char* file);
const char* PollEvent2Str(short int event);
unsigned long NativeThreadID();

#if defined(LIBGO_SYS_Unix)
# define GCC_FORMAT_CHECK __attribute__((format(printf,1,2)))
#else
# define GCC_FORMAT_CHECK
#endif
std::string Format(const char* fmt, ...) GCC_FORMAT_CHECK;
std::string P(const char* fmt, ...) GCC_FORMAT_CHECK;
std::string P();

class ErrnoStore {
    
public:
    ErrnoStore() : restored_(false) {
#if defined(LIBGO_SYS_Windows)
		wsaErr_ = WSAGetLastError();
#endif
		errno_ = errno;
	}
    ~ErrnoStore() {
        Restore();
    }
    void Restore() {
        if (restored_) return ;
        restored_ = true;
#if defined(LIBGO_SYS_Windows)
		WSASetLastError(wsaErr_);
#endif
        errno = errno_;
    }
private:
    int errno_;
#if defined(LIBGO_SYS_Windows)
	int wsaErr_;
#endif
    bool restored_;
};

extern std::mutex gDbgLock;

} //namespace co

#define DebugPrint(type, fmt, ...) \
    do { \
        if (UNLIKELY(::co::CoroutineOptions::getInstance().debug & (type))) { \
            ::co::ErrnoStore es; \
            std::unique_lock<std::mutex> lock(::co::gDbgLock); \
            fprintf(::co::CoroutineOptions::getInstance().debug_output, "[%s][%05d][%04d][%06d]%s:%d:(%s)\t " fmt "\n", \
                    ::co::GetCurrentTimeStr().c_str(),\
                    ::co::GetCurrentProcessID(), ::co::GetCurrentThreadID(), ::co::GetCurrentCoroID(), \
                    ::co::BaseFile(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__); \
            fflush(::co::CoroutineOptions::getInstance().debug_output); \
        } \
    } while(0)


#define LIBGO_E2S_DEFINE(x) \
    case x: return #x
