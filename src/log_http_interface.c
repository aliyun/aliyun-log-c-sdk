#include "log_http_interface.h"
#include <string.h>


LOG_CPP_START

// register external function
__attribute__ ((visibility("default")))
void log_set_http_post_func(int (*f)(const char *url,
                                     char **header_array,
                                     int header_count,
                                     const void *data,
                                     int data_len));


static int (*__LOG_OS_HttpPost)(const char *url,
                                char **header_array,
                                int header_count,
                                const void *data,
                                int data_len) = NULL;

void log_set_http_post_func(int (*f)(const char *url,
                                     char **header_array,
                                     int header_count,
                                     const void *data,
                                     int data_len))
{
    __LOG_OS_HttpPost = f;
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


