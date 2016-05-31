#ifndef LIB_LOG_AUTH_H
#define LIB_LOG_AUTH_H

#include "aos_util.h"
#include "aos_string.h"
#include "aos_http_io.h"
#include "log_define.h"

LOG_CPP_START

/**
  * @brief  sign log headers 
**/
void log_sign_headers(aos_pool_t *p, 
                      const aos_string_t *signstr, 
                      const aos_string_t *access_key_id,
                      const aos_string_t *access_key_secret, 
                      aos_table_t *headers);

/**
  * @brief  get string to signature
**/
int log_get_string_to_sign(aos_pool_t *p, 
                           http_method_e method, 
                           const aos_string_t *canon_res,
                           const aos_table_t *headers, 
                           const aos_table_t *params, 
                           aos_string_t *signstr);

/**
  * @brief  get signed log request headers
**/
int log_get_signed_headers(aos_pool_t *p, const aos_string_t *access_key_id, 
                           const aos_string_t *access_key_secret,
                           const aos_string_t* canon_res, aos_http_request_t *req);

/**
  * @brief  sign log request
**/
int log_sign_request(aos_http_request_t *req, const log_config_t *config);

/**
  * @brief  generate log request Signature
**/
int get_log_request_signature(const log_request_options_t *options, aos_http_request_t *req,
        const aos_string_t *expires, aos_string_t *signature);

LOG_CPP_END

#endif
