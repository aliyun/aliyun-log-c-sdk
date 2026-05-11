#ifdef WIN32

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <new>

#include "log_multi_thread.h"

struct log_windows_mutex
{
    std::mutex mutex;
};

struct log_windows_cond
{
    std::condition_variable cond;
};

extern "C" CRITICALSECTION CreateCriticalSection(void)
{
    try
    {
        return new (std::nothrow) log_windows_mutex();
    }
    catch (...)
    {
        return INVALID_CRITSECT;
    }
}

extern "C" void ReleaseCriticalSection(CRITICALSECTION cs)
{
    delete cs;
}

extern "C" void log_windows_mutex_lock(CRITICALSECTION cs)
{
    if (cs != INVALID_CRITSECT)
    {
        cs->mutex.lock();
    }
}

extern "C" void log_windows_mutex_unlock(CRITICALSECTION cs)
{
    if (cs != INVALID_CRITSECT)
    {
        cs->mutex.unlock();
    }
}

extern "C" COND CreateCond(void)
{
    try
    {
        return new (std::nothrow) log_windows_cond();
    }
    catch (...)
    {
        return INVALID_COND;
    }
}

extern "C" void DeleteCond(COND cond)
{
    delete cond;
}

extern "C" COND_WAIT_T COND_WAIT_TIME(COND cond, CRITICALSECTION cs, int32_t waitMs)
{
    if (cond == INVALID_COND || cs == INVALID_CRITSECT)
    {
        return EINVAL;
    }

    std::unique_lock<std::mutex> lock(cs->mutex, std::adopt_lock);
    COND_WAIT_T result = COND_WAIT_OK;

    try
    {
        if (waitMs < 0)
        {
            cond->cond.wait(lock);
        }
        else if (cond->cond.wait_for(lock, std::chrono::milliseconds(waitMs)) == std::cv_status::timeout)
        {
            result = COND_WAIT_TIMEOUT;
        }
    }
    catch (...)
    {
        result = EINVAL;
    }

    lock.release();
    return result;
}

extern "C" COND_WAIT_T COND_WAKE(COND cond)
{
    if (cond == INVALID_COND)
    {
        return EINVAL;
    }

    cond->cond.notify_one();
    return COND_WAIT_OK;
}

extern "C" COND_WAIT_T COND_WAKE_ALL(COND cond)
{
    if (cond == INVALID_COND)
    {
        return EINVAL;
    }

    cond->cond.notify_all();
    return COND_WAIT_OK;
}

#endif
