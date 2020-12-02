#ifndef SLS_HTTP_INTERFACE_H
#define SLS_HTTP_INTERFACE_H

#include "log_define.h"

LOG_CPP_START

int OS_HttpPost(const char *url,
                char **header_array,
                int header_count,
                const void *data,
                int data_len);
LOG_CPP_END


#endif//SLS_HTTP_INTERFACE_H