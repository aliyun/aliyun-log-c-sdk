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

    int len = snprintf(buffer, 1020, "[%s] [%s:%d] ",
                   _aos_log_level_str[level],
                    file, line);
    
    va_start(args, fmt);
    len += vsnprintf(buffer + len, 1020 - len, fmt, args);
    va_end(args);

    while (buffer[len -1] == '\n') len--;
    buffer[len] = '\0';

    puts(buffer);
}

