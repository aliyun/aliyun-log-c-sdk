#include "log_http_interface.h"
#include "log_define.h"
#include <string.h>
#include <time.h>


LOG_CPP_START

// register external function
__attribute__ ((visibility("default")))
void log_set_http_post_func(int (*f)(const char *url,
                                     char **header_array,
                                     int header_count,
                                     const void *data,
                                     int data_len));

__attribute__ ((visibility("default")))
void log_set_get_time_unix_func(unsigned int (*f)());

__attribute__ ((visibility("default")))
void log_set_http_header_inject_func(void (*f) (log_producer_config *config,
                                               char **src_headers,
                                               int src_count,
                                               char **dest_headers,
                                               int *dest_count)
                                    );
void log_set_http_header_release_inject_func(void (*f) (log_producer_config *config,
                                                char **dest_headers,
                                                int dest_count)
);


__attribute__ ((visibility("default")))
void log_set_http_global_init_func(log_status_t (*f)());

__attribute__ ((visibility("default")))
void log_set_http_global_destroy_func(void (*f)());

static int (*__LOG_OS_HttpPost)(const char *url,
                                char **header_array,
                                int header_count,
                                const void *data,
                                int data_len) = NULL;

static unsigned int (*__LOG_GET_TIME)() = NULL;

static void (*__log_http_header_injector)(log_producer_config *config, char **src_headers, int src_count, char **dest_headers, int *dest_count) = NULL;
static void (*__log_http_header_release_injector)(log_producer_config *config, char **dest_headers, int dest_count) = NULL;
static log_status_t(*__log_http_global_init_func)() = NULL;
static void (*__log_http_global_destroy_func)() = NULL;

void log_set_http_post_func(int (*f)(const char *url,
                                     char **header_array,
                                     int header_count,
                                     const void *data,
                                     int data_len))
{
    __LOG_OS_HttpPost = f;
}

void log_set_get_time_unix_func(unsigned int (*f)())
{
    __LOG_GET_TIME = f;
}

unsigned int LOG_GET_TIME() {
    if (__LOG_GET_TIME == NULL) {
        return time(NULL);
    }
    return __LOG_GET_TIME();
}

int LOG_OS_HttpPost(const char *url,
                    char **header_array,
                    int header_count,
                    const void *data,
                    int data_len)
{
    int (*f)(const char *url,
             char **header_array,
             int header_count,
             const void *data,
             int data_len) = NULL;
    f = __LOG_OS_HttpPost;

    int ret = 506;
    if(f != NULL) {
        ret = f(url, header_array, header_count, data, data_len);
    }
    return ret;
}

void log_set_http_header_inject_func(void (*f) (log_producer_config *config,
                                               char **src_headers,
                                               int src_count,
                                               char **dest_header,
                                               int *dest_count)
                                    )
{
    __log_http_header_injector = f;
}

void log_set_http_header_release_inject_func(void (*f) (log_producer_config *config,
                                                        char **dest_headers,
                                                        int dest_count)
                                    )
{
    __log_http_header_release_injector = f;
}

void log_set_http_global_init_func(log_status_t (*f)())
{
    __log_http_global_init_func = f;
}
void log_set_http_global_destroy_func(void (*f)())
{
    __log_http_global_destroy_func = f;
}

void log_http_inject_headers(log_producer_config *config, char **src_headers, int src_count, char **dest_headers, int *dest_count)
{
    void (*f)(log_producer_config *config, char** src_headers, int src_count, char **dest_headers, int *dest_count) = NULL;
    f = __log_http_header_injector;

    if (NULL != f)
    {
        f(config, src_headers, src_count, dest_headers, dest_count);
    }
}

void log_http_release_inject_headers(log_producer_config *config, char **dest_headers, int dest_count)
{
    void (*f)(log_producer_config *config, char **dest_headers, int dest_count) = NULL;
    f = __log_http_header_release_injector;
    if (NULL != f) {
        f(config, dest_headers, dest_count);
    }
}

log_status_t log_http_global_init()
{
    log_status_t (*f)() = NULL;
    f = __log_http_global_init_func;
    if (NULL != f) {
        return f();
    }
    return 0;
}

void log_http_global_destroy()
{
    void (*f)() = NULL;
    f = __log_http_global_destroy_func;
    if (NULL != f) {
        f();
    }
}

LOG_CPP_END


