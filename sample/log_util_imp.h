#ifndef LOG_UTIL_IMP_H
#define LOG_UTIL_IMP_H

#include "log_util.h"

/**
 * 日志服务依赖的三个函数实现，您可以根据自己系统中提供的库函数实现相应接口
 * 这里实现我们依赖了md5.h sha1.h hmac-sha.h 以及相关实现的源码
 */

LOG_CPP_START

void md5_to_string(const char * buffer, int bufLen, char * md5);

int signature_to_base64(const char * sig, int sigLen, const char * key, int keyLen, char * base64);

void get_now_time_str(char * buffer);

LOG_CPP_END

#endif//LOG_UTIL_IMP_H
