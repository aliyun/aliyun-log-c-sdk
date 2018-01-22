#ifndef LIBLOG_UTIL_H
#define LIBLOG_UTIL_H

#include "log_define.h"
LOG_CPP_START

void md5_to_string(const char * buffer, int bufLen, char * md5);

int signature_to_base64(const char * sig, int sigLen, const char * key, int keyLen, char * base64);

LOG_CPP_END
#endif
