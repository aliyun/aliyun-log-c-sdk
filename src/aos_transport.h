#ifndef LIBAOS_TRANSPORT_H
#define LIBAOS_TRANSPORT_H

#include "aos_define.h"
#include "aos_buf.h"

AOS_CPP_START

typedef struct aos_http_request_s aos_http_request_t;
typedef struct aos_http_response_s aos_http_response_t;
typedef struct aos_http_transport_s aos_http_transport_t;
typedef struct aos_http_controller_s aos_http_controller_t;

typedef struct aos_http_request_options_s aos_http_request_options_t;
typedef struct aos_http_transport_options_s aos_http_transport_options_t;
typedef struct aos_curl_http_transport_s aos_curl_http_transport_t;

typedef int (*aos_read_http_body_pt)(aos_http_request_t *req, char *buffer, int len);
typedef int (*aos_write_http_body_pt)(aos_http_response_t *resp, const char *buffer, int len);

void aos_curl_response_headers_parse(aos_pool_t *p, aos_table_t *headers, char *buffer, int len);
aos_http_transport_t *aos_curl_http_transport_create(aos_pool_t *p);
int aos_curl_http_transport_perform(aos_http_transport_t *t);

struct aos_http_request_options_s {
    int speed_limit;
    int speed_time;
    int dns_cache_timeout;
    int connect_timeout;
    int64_t max_memory_size;
};

struct aos_http_transport_options_s {
    char *user_agent;
    char *cacerts_path;
    uint32_t ssl_verification_disabled:1;
};

#define AOS_HTTP_BASE_CONTROLLER_DEFINE         \
    aos_http_request_options_t *options;        \
    aos_pool_t *pool;                           \
    int64_t start_time;                         \
    int64_t first_byte_time;                    \
    int64_t finish_time;                        \
    uint32_t owner:1;                           \
    void *user_data;

struct aos_http_controller_s {
    AOS_HTTP_BASE_CONTROLLER_DEFINE
};

typedef struct aos_http_controller_ex_s {
    AOS_HTTP_BASE_CONTROLLER_DEFINE
    // private
    int error_code;
    char *reason; // can't modify
} aos_http_controller_ex_t;

typedef enum {
    BODY_IN_MEMORY = 0,
    BODY_IN_FILE,
    BODY_IN_CALLBACK
} aos_http_body_type_e;

struct aos_http_request_s {
    char *host;
    char *proto;
    char *signed_url;
    
    http_method_e method;
    char *uri;
    char *resource;
    aos_table_t *headers;
    aos_table_t *query_params;
    
    aos_list_t body;
    int64_t body_len;
    char *file_path;
    aos_file_buf_t *file_buf;

    aos_pool_t *pool;
    void *user_data;
    aos_read_http_body_pt read_body;

    aos_http_body_type_e type;
};

struct aos_http_response_s {
    int status;
    aos_table_t *headers;

    aos_list_t body;
    int64_t body_len;
    char *file_path;
    aos_file_buf_t* file_buf;
    int64_t content_length;

    aos_pool_t *pool;
    void *user_data;
    aos_write_http_body_pt write_body;

    aos_http_body_type_e type;
};

typedef enum {
    TRANS_STATE_INIT,
    TRANS_STATE_HEADER,
    TRANS_STATE_BODY_IN,
    TRANS_STATE_BODY_OUT,
    TRANS_STATE_ABORT,
    TRANS_STATE_DONE
} aos_transport_state_e;

#define AOS_HTTP_BASE_TRANSPORT_DEFINE           \
    aos_http_request_t *req;                     \
    aos_http_response_t *resp;                   \
    aos_pool_t *pool;                            \
    aos_transport_state_e state;                 \
    aos_array_header_t *cleanup;                 \
    aos_http_transport_options_t *options;       \
    aos_http_controller_ex_t *controller;
    
struct aos_http_transport_s {
    AOS_HTTP_BASE_TRANSPORT_DEFINE
};

struct aos_curl_http_transport_s {
    AOS_HTTP_BASE_TRANSPORT_DEFINE
    CURL *curl;
    char *url;
    struct curl_slist *headers;
    curl_read_callback header_callback;
    curl_read_callback read_callback;
    curl_write_callback write_callback;
};

AOS_CPP_END

#endif
