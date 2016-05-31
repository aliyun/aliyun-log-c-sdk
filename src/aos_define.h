#ifndef LIBAOS_DEFINE_H
#define LIBAOS_DEFINE_H

#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>

#include <curl/curl.h>
#include <apr_time.h>
#include <apr_strings.h>
#include <apr_pools.h>
#include <apr_tables.h>
#include <apr_file_io.h>

#ifdef __cplusplus
# define AOS_CPP_START extern "C" {
# define AOS_CPP_END }
#else
# define AOS_CPP_START
# define AOS_CPP_END
#endif

typedef enum {
    HTTP_GET,
    HTTP_HEAD,
    HTTP_PUT,
    HTTP_POST,
    HTTP_DELETE
} http_method_e;

typedef enum {
    AOSE_OK = 0,
    AOSE_OUT_MEMORY = -1000,
    AOSE_OVER_MEMORY = -999,
    AOSE_FAILED_CONNECT = -998,
    AOSE_ABORT_CALLBACK = -997,
    AOSE_INTERNAL_ERROR = -996,
    AOSE_REQUEST_TIMEOUT = -995,
    AOSE_INVALID_ARGUMENT = -994,
    AOSE_INVALID_OPERATION = -993,
    AOSE_CONNECTION_FAILED = -992,
    AOSE_FAILED_INITIALIZE = -991,
    AOSE_NAME_LOOKUP_ERROR = -990,
    AOSE_FAILED_VERIFICATION = -989,
    AOSE_WRITE_BODY_ERROR = -988,
    AOSE_READ_BODY_ERROR = -987,
    AOSE_SERVICE_ERROR = -986,
    AOSE_OPEN_FILE_ERROR = -985,
    AOSE_FILE_SEEK_ERROR = -984,
    AOSE_FILE_INFO_ERROR = -983,
    AOSE_FILE_READ_ERROR = -982,
    AOSE_FILE_WRITE_ERROR = -981,
    AOSE_XML_PARSE_ERROR = -980,
    AOSE_UTF8_ENCODE_ERROR = -979,
    AOSE_UNKNOWN_ERROR = -978
} aos_error_code_e;

typedef apr_pool_t aos_pool_t;
typedef apr_table_t aos_table_t;
typedef apr_table_entry_t aos_table_entry_t;
typedef apr_array_header_t aos_array_header_t;

#define aos_table_elts(t) apr_table_elts(t)
#define aos_is_empty_table(t) apr_is_empty_table(t)
#define aos_table_make(p, n) apr_table_make(p, n)
#define aos_table_add_int(t, key, value) do {       \
        char value_str[64];                             \
        snprintf(value_str, sizeof(value_str), "%d", value);\
        apr_table_add(t, key, value_str);               \
    } while(0)

#define aos_table_add_int64(t, key, value) do {       \
        char value_str[64];                             \
        snprintf(value_str, sizeof(value_str), "%" APR_INT64_T_FMT, value);\
        apr_table_add(t, key, value_str);               \
    } while(0)

#define aos_table_set_int64(t, key, value) do {       \
        char value_str[64];                             \
        snprintf(value_str, sizeof(value_str), "%" APR_INT64_T_FMT, value);\
        apr_table_set(t, key, value_str);               \
    } while(0)

#define aos_pool_create(n, p) apr_pool_create(n, p)
#define aos_pool_destroy(p) apr_pool_destroy(p)
#define aos_palloc(p, s) apr_palloc(p, s)
#define aos_pcalloc(p, s) apr_pcalloc(p, s)

#define AOS_INIT_WINSOCK 1
#define AOS_MD5_STRING_LEN 32
#define AOS_MAX_URI_LEN 2048
#define AOS_MAX_HEADER_LEN 8192
#define AOS_MAX_QUERY_ARG_LEN 1024
#define AOS_MAX_GMT_TIME_LEN 128

#define AOS_CONNECT_TIMEOUT 10
#define AOS_DNS_CACHE_TIMOUT 60
#define AOS_MIN_SPEED_LIMIT 1024
#define AOS_MIN_SPEED_TIME 15
#define AOS_MAX_MEMORY_SIZE 1024*1024*1024L;

#define AOS_REQUEST_STACK_SIZE 32

#define aos_abs(value)       (((value) >= 0) ? (value) : - (value))
#define aos_max(val1, val2)  (((val1) < (val2)) ? (val2) : (val1))
#define aos_min(val1, val2)  (((val1) > (val2)) ? (val2) : (val1))

#define LF     (char) 10
#define CR     (char) 13
#define CRLF   "\x0d\x0a"

#define AOS_VERSION    "2.1.0"
#define AOS_VER        "aliyun-sdk-c/" AOS_VERSION

#define AOS_HTTP_PREFIX   "http://"
#define AOS_HTTPS_PREFIX  "https://"

#endif
