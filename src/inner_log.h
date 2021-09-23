#ifndef LIBAOS_LOG_H
#define LIBAOS_LOG_H
#include <string.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum {
    AOS_LOG_OFF = 1,
    AOS_LOG_FATAL,
    AOS_LOG_ERROR,
    AOS_LOG_WARN,
    AOS_LOG_INFO,
    AOS_LOG_DEBUG,
    AOS_LOG_TRACE,
    AOS_LOG_ALL
} aos_log_level_e;

extern aos_log_level_e aos_log_level;

void aos_log_format(int level,
                     const char *file,
                     int line,
                     const char *function,
                     const char *fmt, ...);

void aos_print_log_android(int level, char *log);

void aos_log_set_level(aos_log_level_e level);
#ifdef __ANDROID__
#define print_log(level, log) aos_print_log_android(level, log)
#else
#define print_log(level, log) puts(log)
#endif

#ifdef WIN32
#define __FL_NME__ (strchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#define aos_fatal_log(format, ...) if(aos_log_level>=AOS_LOG_FATAL) \
        aos_log_format(AOS_LOG_FATAL, __FL_NME__, __LINE__, __FUNCTION__, format, __VA_ARGS__)
#define aos_error_log(format, ...) if(aos_log_level>=AOS_LOG_ERROR) \
        aos_log_format(AOS_LOG_ERROR, __FL_NME__, __LINE__, __FUNCTION__, format, __VA_ARGS__)
#define aos_warn_log(format, ...) if(aos_log_level>=AOS_LOG_WARN)   \
        aos_log_format(AOS_LOG_WARN, __FL_NME__, __LINE__, __FUNCTION__, format, __VA_ARGS__)
#define aos_info_log(format, ...) if(aos_log_level>=AOS_LOG_INFO)   \
        aos_log_format(AOS_LOG_INFO, __FL_NME__, __LINE__, __FUNCTION__, format, __VA_ARGS__)
#define aos_debug_log(format, ...) if(aos_log_level>=AOS_LOG_DEBUG) \
        aos_log_format(AOS_LOG_DEBUG, __FL_NME__, __LINE__, __FUNCTION__, format, __VA_ARGS__)
#define aos_trace_log(format, ...) if(aos_log_level>=AOS_LOG_TRACE) \
        aos_log_format(AOS_LOG_TRACE, __FL_NME__, __LINE__, __FUNCTION__, format, __VA_ARGS__)
#else
#define __FL_NME__ (strchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define aos_fatal_log(format, args...) if(aos_log_level>=AOS_LOG_FATAL) \
        aos_log_format(AOS_LOG_FATAL, __FL_NME__, __LINE__, __FUNCTION__, format, ## args)
#define aos_error_log(format, args...) if(aos_log_level>=AOS_LOG_ERROR) \
        aos_log_format(AOS_LOG_ERROR, __FL_NME__, __LINE__, __FUNCTION__, format, ## args)
#define aos_warn_log(format, args...) if(aos_log_level>=AOS_LOG_WARN)   \
        aos_log_format(AOS_LOG_WARN, __FL_NME__, __LINE__, __FUNCTION__, format, ## args)
#define aos_info_log(format, args...) if(aos_log_level>=AOS_LOG_INFO)   \
        aos_log_format(AOS_LOG_INFO, __FL_NME__, __LINE__, __FUNCTION__, format, ## args)
#define aos_debug_log(format, args...) if(aos_log_level>=AOS_LOG_DEBUG) \
        aos_log_format(AOS_LOG_DEBUG, __FL_NME__, __LINE__, __FUNCTION__, format, ## args)
#define aos_trace_log(format, args...) if(aos_log_level>=AOS_LOG_TRACE) \
        aos_log_format(AOS_LOG_TRACE, __FL_NME__, __LINE__, __FUNCTION__, format, ## args)
#endif

#ifdef __cplusplus
}
#endif

#endif
