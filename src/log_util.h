#ifndef LOG_UTIL_H
#define LOG_UTIL_H

#include "log_define.h"
LOG_CPP_START

/**
 * compute buffer's md5 and cast to a hex based md5 string(upper)
 *
 * @param buffer data buffer
 * @param bufLen data buffer length
 * @param md5 the buffer to store hex md5 string, at least 32 bytes
 */
typedef void (*md5_to_string_fun)(const char * buffer, int bufLen, char * md5);

/**
 * compute signature's hash with hmac-sha1, and encode this hash to base64
 *
 * @param sig signature buffer
 * @param sigLen signature buffer length
 * @param key key buffer
 * @param keyLen key buffer length
 * @param base64 the buffer to store base64 encoded string, at least 128 bytes
 * @return length of base64 encoded string
 */
typedef int (*hmac_sha1_signature_to_base64_fun)(const char * sig, int sigLen, const char * key, int keyLen, char * base64);

/**
 * get now time string, time fmt '%a, %d %b %Y %H:%M:%S GMT' , RFC 822, like 'Mon, 3 Jan 2010 08:33:47 GMT'
 *
 * @param buffer the buffer to store time str, at least 64 bytes
 */
typedef void (*get_now_time_str_fun)(char * buffer);

/**
 * users should set these functions ptr to make aliyun log sdk work
 * @note you must set these ptr, not define it
 */
extern md5_to_string_fun g_log_md5_to_string_fun;
extern hmac_sha1_signature_to_base64_fun g_log_hmac_sha1_signature_to_base64_fun;
extern get_now_time_str_fun g_log_get_now_time_str_fun;

LOG_CPP_END
#endif
