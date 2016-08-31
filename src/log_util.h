#ifndef LIBLOG_UTIL_H
#define LIBLOG_UTIL_H

#include "aos_util.h"
#include "aos_string.h"
#include "aos_status.h"
#include "log_define.h"
LOG_CPP_START

int starts_with(const aos_string_t *str, const char *prefix);

void generate_proto(const log_request_options_t *options, aos_http_request_t *req);

log_config_t *log_config_create(aos_pool_t *p);

void log_post_logs_uri(const log_request_options_t *options, const aos_string_t *project, const aos_string_t *logstore, aos_http_request_t *req);

log_request_options_t *log_request_options_create(aos_pool_t *p);

void log_write_request_body_from_buffer(aos_list_t *buffer, aos_http_request_t *req);

void log_post_logs_request(const log_request_options_t *options, 
        const aos_string_t *project,
        const aos_string_t *logstore, 
        http_method_e method, 
        aos_http_request_t **req, 
        aos_table_t *params, 
        aos_table_t *headers,
        aos_http_response_t **resp);

aos_status_t *log_send_request(aos_http_controller_t *ctl, 
        aos_http_request_t *req,
        aos_http_response_t *resp);

aos_status_t *log_process_request(const log_request_options_t *options,
        aos_http_request_t *req,
        aos_http_response_t *resp);

aos_status_t *log_post_logs_from_buffer(const log_request_options_t *options,
        const aos_string_t *project,
        const aos_string_t *logstore, 
        aos_list_t *buffer,
        aos_table_t *headers, 
        aos_table_t **resp_headers);
aos_table_t* aos_table_create_if_null(const log_request_options_t *options, aos_table_t *table, int table_size);
void log_init_request(const log_request_options_t *options,
                      http_method_e method,
                      aos_http_request_t **req,
                      aos_table_t *params,
                      aos_table_t *headers,
                      aos_http_response_t **resp);
LOG_CPP_END
#endif
