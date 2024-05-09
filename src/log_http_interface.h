#ifndef SLS_HTTP_INTERFACE_H
#define SLS_HTTP_INTERFACE_H

#include "log_define.h"
#include "log_producer_config.h"

#ifdef _WIN32
    #define LOG_EXPORT_API __declspec(dllexport)
#else
  #define LOG_EXPORT_API __attribute__((visibility("default")))
#endif

LOG_CPP_START

/**
 * register http post function for sending logs
 * @param f function ptr to send http post request
 */
LOG_EXPORT_API
void log_set_http_post_func(int (*f)(const char *url,
                                     char **header_array,
                                     int header_count,
                                     const void *data,
                                     int data_len));

/**
 * register get time function for get log time
 * @param f function ptr to get time unix seconds, like time(NULL)
 */
LOG_EXPORT_API
void log_set_get_time_unix_func(unsigned int (*f)());

LOG_EXPORT_API
void log_set_http_header_inject_func(void (*f) (log_producer_config *config,
                                                char **src_headers,
                                                int src_count,
                                                char **dest_headers,
                                                int *dest_count)
);

LOG_EXPORT_API
void log_set_http_header_release_inject_func(void (*f) (log_producer_config *config,
                                                        char **dest_headers,
                                                        int dest_count)
);

/**
 * register http global init function
 * @param f function ptr to do http global init
 */
LOG_EXPORT_API
void log_set_http_global_init_func(log_status_t (*f)());


/**
 * register http global destroy function
 * @param f function ptr to do http global destroy
 */
LOG_EXPORT_API
void log_set_http_global_destroy_func(void (*f)());

LOG_CPP_END


#endif//SLS_HTTP_INTERFACE_H