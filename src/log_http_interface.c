#include "log_http_interface.h"
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


static int (*__LOG_OS_HttpPost)(const char *url,
                                char **header_array,
                                int header_count,
                                const void *data,
                                int data_len) = NULL;

static unsigned int (*__LOG_GET_TIME)() = NULL;



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

LOG_CPP_END


