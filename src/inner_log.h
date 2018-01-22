#ifndef LIBAOS_LOG_H
#define LIBAOS_LOG_H

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

#define aos_fatal_log(format, args...) if(aos_log_level>=AOS_LOG_FATAL) \
        aos_log_format(AOS_LOG_FATAL, __FILE__, __LINE__, __FUNCTION__, format, ## args)
#define aos_error_log(format, args...) if(aos_log_level>=AOS_LOG_ERROR) \
        aos_log_format(AOS_LOG_ERROR, __FILE__, __LINE__, __FUNCTION__, format, ## args)
#define aos_warn_log(format, args...) if(aos_log_level>=AOS_LOG_WARN)   \
        aos_log_format(AOS_LOG_WARN, __FILE__, __LINE__, __FUNCTION__, format, ## args)
#define aos_info_log(format, args...) if(aos_log_level>=AOS_LOG_INFO)   \
        aos_log_format(AOS_LOG_INFO, __FILE__, __LINE__, __FUNCTION__, format, ## args)
#define aos_debug_log(format, args...) if(aos_log_level>=AOS_LOG_DEBUG) \
        aos_log_format(AOS_LOG_DEBUG, __FILE__, __LINE__, __FUNCTION__, format, ## args)
#define aos_trace_log(format, args...) if(aos_log_level>=AOS_LOG_TRACE) \
        aos_log_format(AOS_LOG_TRACE, __FILE__, __LINE__, __FUNCTION__, format, ## args)

#ifdef __cplusplus
}
#endif

#endif
