#include "inner_log.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

aos_log_level_e aos_log_level = AOS_LOG_WARN;

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


void aos_log_format(int level,
                    const char *file,
                    int line,
                    const char *function,
                    const char *fmt, ...)
{
    va_list args;
    char buffer[1024];
    int maxLen = 1020;

    // @note return value maybe < 0 || > maxLen
    int len = snprintf(buffer, maxLen, "[%s] [%s][%s:%d] ",
                       _aos_log_level_str[level],
                       file, function, line);
    // should never happen
    if (len < 0 || len > maxLen) {
        print_log(AOS_LOG_ERROR, "[aos_log_format] error log fmt\n");
        return;
    }

    va_start(args, fmt);
    // @note return value maybe < 0 || > maxLen
    int rst = vsnprintf(buffer + len, maxLen - len, fmt, args);
    va_end(args);
    if (rst < 0) {
        print_log(AOS_LOG_ERROR, "[aos_log_format] error log fmt\n");
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

    print_log(level, buffer);
}