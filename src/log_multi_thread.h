#ifndef LOG_MULTI_THREAD_UTIL_H
#define LOG_MULTI_THREAD_UTIL_H

#include "log_inner_include.h"


//不同操作系统资源相关的工具宏定义
#ifdef WIN32
/// * @brief    临界区资源
typedef LPCRITICAL_SECTION CRITICALSECTION;
#define INVALID_CRITSECT NULL

/**
 ********************************************************************
 * 创建互斥锁
 ********************************************************************
 */
static inline CRITICALSECTION CreateCriticalSection()
{
    CRITICALSECTION cs = (CRITICAL_SECTION *)malloc(sizeof(CRITICAL_SECTION));
    InitializeCriticalSection(cs);
    return cs;
}

/**
 ********************************************************************
 * 删除互斥锁
 ********************************************************************
 */
static inline void DeleteCriticalSection(CRITICALSECTION cs) {
    if (cs != INVALID_CRITSECT) {
        DeleteCriticalSection(cs);
        free(cs);
    }
}

/// * @brief    加锁
#define CS_ENTER(cs) EnterCriticalSection(cs)
/// * @brief    解锁
#define CS_LEAVE(cs) LeaveCriticalSection(cs)

/// * @brief    互斥锁
#define MUTEX CRITICAL_SECTION
/// * @brief    加锁
#define MUTEX_LOCK(mutex) EnterCriticalSection(&mutex)
/// * @brief    解锁
#define MUTEX_UNLOCK(mutex) LeaveCriticalSection(&mutex)
/// * @brief    互斥锁初始化
#define MUTEX_INIT(mutex) InitializeCriticalSection(&mutex)
/// * @brief    互斥锁销毁
#define MUTEX_DESTROY(mutex) DeleteCriticalSection(&mutex)

//信号量资源
/// * @brief    信号量
typedef HANDLE SEMA;
/// * @brief    等待信号量一定时间
#define SEMA_WAIT_TIME(sema, delay) WaitForSingleObject(sema, delay)
/// * @brief    一直阻塞地进行等待信号量
#define SEMA_WAIT(sema) WaitForSingleObject(sema, INFINITE)
/// * @brief    释放信号量
#define SEMA_POST(sema) ReleaseSemaphore(sema, 1, NULL)
/// * @brief    尝试获取一个信号量
#define SEMA_TRYWAIT(sema) WaitForSingleObject(sema, 0)
/// * @brief    销毁信号量
#define SEMA_DESTROY(sema) CloseHandle(sema)
/// * @brief    初始化信号量， 输入的为：信号量的最大值，初始信号量个数
#define SEMA_INIT(sema, initCount, maxCount) sema = CreateSemaphore(NULL, initCount, maxCount, NULL)
/// * @brief    初始一个带有名称的信号量，用于多进程交互
#define SEMA_INIT_NAME(sema, initCount, maxCount, semaName) sema = CreateSemaphore(NULL, initCount, maxCount, semaName)
/// * @brief    信号量等待超时
#define SEMA_WAIT_TIMEOUT WAIT_TIMEOUT
/// * @brief    等待到信号量
#define SEMA_WAIT_OK WAIT_OBJECT_0

typedef HANDLE THREAD

#define snprintf sprintf_s

#define ATOMICINT volatile long

#define ATOMICINT_INC(pAtopicInt) InterlockedIncrement(pAtopicInt)
#define ATOMICINT_DEC(pAtopicInt) InterlockedDecrement(pAtopicInt)
#define ATOMICINT_ADD(pAtopicInt, addVal) InterlockedAdd(pAtopicInt, addVal)
#define ATOMICINT_EXCHANGEADD(pAtopicInt, addVal) InterlockedExchangeAdd(pAtopicInt, addVal)
#define ATOMICINT_EXCHANGE(pAtopicInt, exchangeVal) InterlockedExchange(pAtopicInt, exchangeVal)
#define ATOMICINT_COMPAREEXCAHNGE(pAtopicInt, exchangeVal, cmpVal) InterlockedCompareExchange(pAtopicInt, exchangeVal, cmpVal)

#elif defined(_VXWORKS)

//临界区资源
typedef SEM_ID CRITICALSECTION;
#define INVALID_CRITSECT NULL
static inline CRITICALSECTION CreateCriticalSection(int spinCount = 0)
{
    CRITICALSECTION cs = semMCreate(SEM_Q_PRIORITY | SEM_DELETE_SAFE | SEM_INVERSION_SAFE);
    if (cs == NULL)
    {
        perror("vxworks create MUTUAL EXCLUSION SEMAPHORE failed\n");
    }
    return cs;
}

static inline void DeleteCriticalSection(CRITICALSECTION & cs)
{
    semDelete(cs);
    cs = INVALID_CRITSECT;
}

#define CS_ENTER(cs) semTake(cs, WAIT_FOREVER)
#define CS_LEAVE(cs) semGive(cs)

#define MUTEX SEM_ID
#define MUTEX_LOCK(mutex) semTake(mutex, WAIT_FOREVER)
#define MUTEX_UNLOCK(mutex) semGive(mutex)
#define MUTEX_INIT(mutex) mutex = semBCreate(SEM_Q_FIFO,SEM_FULL)
#define MUTEX_DESTROY(mutex) semDelete(mutex)
//信号量资源
#define SEMA SEM_ID
#define SEMA_WAIT_TIME(sema,delay) semTake(sema, delay)
#define SEMA_WAIT(sema) semTake(sema, WAIT_FOREVER)
#define SEMA_POST(sema) semGive(sema)
#define SEMA_DESTROY(sema) semDelete(sema)
#define SEMA_INIT(sema, initCount, maxCount) sema = semCCreate(SEM_Q_FIFO,initCount)
#define SEMA_WAIT_TIMEOUT ERROR
//线程资源
#define THREADID int
#define SOCKET int
#define closesocket(s_) close(s_)
#define SOCKET_ERROR -1

#else

//临界区资源
typedef pthread_mutex_t* CRITICALSECTION;
#define INVALID_CRITSECT NULL

static inline CRITICALSECTION CreateCriticalSection() {
    CRITICALSECTION cs = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    assert(cs != INVALID_CRITSECT);
    pthread_mutex_init(cs, NULL);
    return cs;
}

static inline void DeleteCriticalSection(CRITICALSECTION cs) {
    if (cs != INVALID_CRITSECT) {
        pthread_mutex_destroy(cs);
        free(cs);
    }
}

#define CS_ENTER(cs) pthread_mutex_lock(cs)
#define CS_LEAVE(cs) pthread_mutex_unlock(cs)

typedef pthread_cond_t* COND;
typedef int COND_WAIT_T;
#define COND_WAIT_OK 0
#define COND_WAIT_TIMEOUT ETIMEDOUT

#define INVALID_COND NULL

static inline COND CreateCond() {
    COND cond = (COND)malloc(sizeof(pthread_cond_t));
    assert(cond != INVALID_CRITSECT);
    pthread_cond_init(cond, NULL);
    return cond;
}

static inline void DeleteCond(COND cond) {
    if (cond != INVALID_COND) {
        pthread_cond_destroy(cond);
        free(cond);
    }
}

#define COND_SIGNAL(cond) pthread_cond_signal(cond)
#define COND_SIGNAL_ALL(cond) pthread_cond_broadcast(cond)

static inline COND_WAIT_T COND_WAIT_TIME(COND cond, CRITICALSECTION cs, int32_t waitMs) {
    struct timeval now;
    struct timespec outTime;
    gettimeofday(&now, NULL);

    now.tv_usec += ((waitMs) % 1000) * 1000;
    if (now.tv_usec > 1000000)
    {
        now.tv_usec -= 1000000;
        ++now.tv_sec;
    }
    outTime.tv_sec = now.tv_sec + (waitMs) / 1000;
    outTime.tv_nsec = now.tv_usec * 1000;
    return pthread_cond_timedwait(cond, cs, &outTime);
}

static inline int64_t GET_TIME_US() {
    struct timeval now;
    gettimeofday(&now, NULL);
    return (int64_t)now.tv_sec * 1000000 + now.tv_usec;
}


#define MUTEX pthread_mutex_t
#define SEMA sem_t
#define MUTEX_LOCK(mutex) pthread_mutex_lock(&mutex)
#define MUTEX_UNLOCK(mutex) pthread_mutex_unlock(&mutex)
#define MUTEX_INIT(mutex) pthread_mutex_init(&mutex,NULL)
#define MUTEX_DESTROY(mutex) pthread_mutex_destroy(&mutex)

// not supported in mac os
#ifdef __linux__
static inline int sema_wait_time_(sem_t* sema, unsigned int delayMs)
{
    struct timespec ts;

    struct timeval tv;

    gettimeofday(&tv, NULL);
    tv.tv_usec += (delayMs % 1000) * 1000;
    tv.tv_sec += delayMs / 1000;
    if (tv.tv_usec > 1000000) {
        tv.tv_usec -= 1000000;
        ++tv.tv_sec;
    }

    ts.tv_sec = tv.tv_sec;
    ts.tv_nsec = tv.tv_usec * 1000;

    return sem_timedwait(sema, &ts) == 0 ? 0 : ETIMEDOUT;
}

#define SEMA_WAIT_TIME(sema,delay) sema_wait_time_(&sema,delay)

#endif

#define SEMA_WAIT(sema) sem_wait(&sema)
#define SEMA_POST(sema) sem_post(&sema)
#define SEMA_TRYWAIT(sema) sem_trywait(&sema)
#define SEMA_DESTROY(sema) sem_destroy(&sema)
#define SEMA_INIT(sema, initCount, maxCount) sem_init(&sema,0,initCount)
#define SEMA_INIT_NAME(sema, initCount, maxCount, semaName) sema = sem_open(semaName, O_CREAT, 0, initCount)
#define WAIT_OBJECT_0 0
#define WAIT_TIMEOUT ETIMEDOUT
#define SEMA_WAIT_TIMEOUT ETIMEDOUT
#define SEMA_WAIT_OK 0

typedef pthread_t THREAD;
#define THREAD_INIT(thread, func, param) pthread_create(&(thread), NULL, func, param)
#define THREAD_JOIN(thread) pthread_join(thread, NULL)

#define ATOMICINT volatile long

#define ATOMICINT_INC(pAtopicInt) __sync_add_and_fetch(pAtopicInt, 1)
#define ATOMICINT_DEC(pAtopicInt) __sync_add_and_fetch(pAtopicInt, -1)
#define ATOMICINT_ADD(pAtopicInt, addVal) __sync_add_and_fetch(pAtopicInt, addVal)
#define ATOMICINT_EXCHANGEADD(pAtopicInt, addVal) __sync_fetch_and_add(pAtopicInt, addVal)
#define ATOMICINT_EXCHANGE(pAtopicInt, exchangeVal) __sync_val_compare_and_swap(pAtopicInt, *pAtopicInt, exchangeVal)
#define ATOMICINT_COMPAREEXCAHNGE(pAtopicInt, exchangeVal, cmpVal) __sync_val_compare_and_swap(pAtopicInt, cmpVal, exchangeVal)

typedef struct _FILETIME
{
    unsigned long dwLowDateTime;
    unsigned long dwHighDateTime;
} FILETIME;

#endif //WIN32

#endif //LOG_MULTI_THREAD_UTIL_H
