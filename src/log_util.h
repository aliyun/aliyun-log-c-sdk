#ifndef LIBLOG_UTIL_H
#define LIBLOG_UTIL_H

#include "log_define.h"
#include "sha256.h"
#include <stdint.h> 
LOG_CPP_START

void md5_to_string(const char * buffer, int bufLen, char * md5);

int signature_to_base64(const char * sig, int sigLen, const char * key, int keyLen, char * base64);

void sha256_to_hex_string(uint8_t hash[32], char hex_str[65]);
char *sha256_hash_to_hex(const char *input, uint32 length);

LOG_CPP_END
#endif
