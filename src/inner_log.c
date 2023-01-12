#include "inner_log.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>


void aos_log_print_default(const char *message, int lens)
{
    puts(message);
}


void aos_log_format_default(int level,
                            const char *file,
                            int line,
                            const char *function,
                            const char *fmt, ...);

aos_log_level_e aos_log_level = AOS_LOG_WARN;
aos_log_format_pt aos_log_format = aos_log_format_default;
aos_log_print_pt aos_log_output = aos_log_print_default;


static const char * _aos_log_level_str[] = {
        "NONE",
        "NONE",
        "FATAL",
        "ERROR",
        "WARN",
        "INFO",
        "DEBUG",
        "TRACE",
        "NONE"
};


void aos_log_set_level(aos_log_level_e level)
{   
    aos_log_level = level;
}


void aos_log_format_default(int level,
                    const char *file,
                    int line,
                    const char *function,
                    const char *fmt, ...)
{
    va_list args;
    char buffer[1024];
    int maxLen = 1020;

    // @note return value maybe < 0 || > maxLen
    int len = snprintf(buffer, maxLen, "[%s] [%s:%d] ",
                       _aos_log_level_str[level],
                       file, line);
    // should never happen
    if (len < 0 || len > maxLen) {
        puts("[aos_log_format] error log fmt\n");
        return;
    }

    va_start(args, fmt);
    // @note return value maybe < 0 || > maxLen
    int rst = vsnprintf(buffer + len, maxLen - len, fmt, args);
    va_end(args);
    if (rst < 0) {
        puts("[aos_log_format] error log fmt\n");
        return;
    }

    if (rst > maxLen - len) {
        rst = maxLen - len;
    }
    len += rst;


    while (len > 0 && buffer[len -1] == '\n')
    {
        len--;
    }
    buffer[len++] = '\n';
    buffer[len] = '\0';

    aos_log_output(buffer, len);
}

void aos_log_set_format_callback(aos_log_format_pt func)
{
    if (func == NULL) {
        return;
    }
    aos_log_format = func;
}

void aos_log_set_print_callback(aos_log_print_pt func)
{
    if (func == NULL) {
        return;
    }
    aos_log_output = func;
}
