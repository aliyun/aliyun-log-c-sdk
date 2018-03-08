#ifndef LIBLOG_DEFINE_H
#define LIBLOG_DEFINE_H

#include "aos_string.h"
#include "aos_list.h"
#include "aos_transport.h"

#ifdef __cplusplus
# define LOG_CPP_START extern "C" {
# define LOG_CPP_END }
#else
# define LOG_CPP_START
# define LOG_CPP_END
#endif

LOG_CPP_START

#define aos_xml_error_status_set(STATUS, RES) do {                   \
        aos_status_set(STATUS, RES, AOS_XML_PARSE_ERROR_CODE, NULL); \
    } while(0)

#define aos_file_error_status_set(STATUS, RES) do {                   \
        aos_status_set(STATUS, RES, AOS_OPEN_FILE_ERROR_CODE, NULL); \
    } while(0)

extern const char LOG_CANNONICALIZED_HEADER_PREFIX[];
extern const char ACS_CANNONICALIZED_HEADER_PREFIX[];
extern const char LOG_CANNONICALIZED_HEADER_DATE[];
extern const char LOG_API_VERSION[];
extern const char LOG_COMPRESS_TYPE[];
extern const char LOG_BODY_RAW_SIZE[];
extern const char LOG_SIGNATURE_METHOD[];
extern const char LOG_REQUEST_ID[];
extern const char ACS_SECURITY_TOKEN[];
extern const char LOG_CONTENT_MD5[];
extern const char LOG_CONTENT_TYPE[];
extern const char LOG_CONTENT_LENGTH[];
extern const char LOG_DATE[];
extern const char LOG_AUTHORIZATION[];

typedef struct log_lib_curl_initializer_s log_lib_curl_initializer_t;

typedef struct {
    aos_string_t endpoint;
    aos_string_t access_key_id;
    aos_string_t access_key_secret;
    aos_string_t sts_token;
} log_config_t;

typedef struct {
    log_config_t *config;
    aos_http_controller_t *ctl; /*< aos http controller, more see aos_transport.h */
    aos_pool_t *pool;
} log_request_options_t;

LOG_CPP_END

#endif
