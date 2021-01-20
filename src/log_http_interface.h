#ifndef SLS_HTTP_INTERFACE_H
#define SLS_HTTP_INTERFACE_H

#include "log_define.h"

LOG_CPP_START

/**
 * register http post function for sending logs
 * @param f function ptr to send http post request
 */
__attribute__ ((visibility("default")))
void log_set_http_post_func(int (*f)(const char *url,
                                     char **header_array,
                                     int header_count,
                                     const void *data,
                                     int data_len));

/**
 * register get time function for get log time
 * @param f function ptr to get time unix seconds, like time(NULL)
 */
__attribute__ ((visibility("default")))
void log_set_get_time_unix_func(unsigned int (*f)());

LOG_CPP_END


#endif//SLS_HTTP_INTERFACE_H