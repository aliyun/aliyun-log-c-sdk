#ifndef LIBLOG_DEFINE_H
#define LIBLOG_DEFINE_H


#ifdef WIN32
#define LOG_EXPORT _declspec(dllexport)
#else
#define LOG_EXPORT
#endif

#ifdef __cplusplus
# define LOG_CPP_START extern "C" {
# define LOG_CPP_END }
#else
# define LOG_CPP_START
# define LOG_CPP_END
#endif

#define LOG_GLOBAL_SSL (1<<0)
#define LOG_GLOBAL_WIN32 (1<<1)
#define LOG_GLOBAL_ALL (LOG_GLOBAL_SSL|LOG_GLOBAL_WIN32)
#define LOG_GLOBAL_NOTHING (0)

typedef int log_status_t;

struct _post_log_result
{
    int statusCode;
    char * errorMessage;
    char * requestID;
};

typedef struct _post_log_result post_log_result;


#endif
