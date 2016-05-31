#include "aos_log.h"
#include "apr_portable.h"

aos_log_print_pt  aos_log_print = aos_log_print_default;
aos_log_format_pt aos_log_format = aos_log_format_default;
aos_log_level_e   aos_log_level = AOS_LOG_WARN;

extern apr_file_t *aos_stderr_file;

void aos_log_set_print(aos_log_print_pt p)
{
    aos_log_print = p;
}

void aos_log_set_format(aos_log_format_pt p)
{
    aos_log_format = p;
}

void aos_log_set_level(aos_log_level_e level)
{   
    aos_log_level = level;
}

void aos_log_set_output(apr_file_t *output)
{
    aos_stderr_file = output;
}

void aos_log_print_default(const char *message, int len)
{
    if (aos_stderr_file == NULL) {
        fprintf(stderr, "%s", message);
    } else {
        apr_size_t bnytes = len;
        apr_file_write(aos_stderr_file, message, &bnytes);
    }
}

void aos_log_format_default(int level,
                            const char *file,
                            int line,
                            const char *function,
                            const char *fmt, ...)
{
    int len;
    apr_time_t t;
    int s;
    apr_time_exp_t tm;
    va_list args;
    char buffer[4096];

    t = apr_time_now();
    if ((s = apr_time_exp_lt(&tm, t)) != APR_SUCCESS) {
        return;
    }
    
    len = snprintf(buffer, 4090, "[%04d-%02d-%02d %02d:%02d:%02d.%03d] %" APR_INT64_T_FMT " %s:%d ",
                   tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                   tm.tm_hour, tm.tm_min, tm.tm_sec, tm.tm_usec/1000,
                   (int64_t)apr_os_thread_current(), file, line);
    
    va_start(args, fmt);
    len += vsnprintf(buffer + len, 4090 - len, fmt, args);
    va_end(args);

    while (buffer[len -1] == '\n') len--;
    buffer[len++] = '\n';
    buffer[len] = '\0';

    aos_log_print(buffer, len);
}

