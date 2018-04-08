#include "lz4.h"
#include "aos_log.h"
#include "aos_util.h"
#include "aos_string.h"
#include "aos_status.h"
#include "log_auth.h"
#include "log_util.h"
#include "log_api.h"

static void _copy_option(log_post_option * logPostOption, log_request_options_t *options){
    if (logPostOption != NULL){
        if (logPostOption->interface != NULL) {
            options->ctl->interface = (char *)logPostOption->interface;
        }
        if (logPostOption->connect_timeout > 0) {
            options->ctl->connect_timeout = logPostOption->connect_timeout;
        }
        if (logPostOption->operation_timeout > 0) {
            options->ctl->operation_timeout = logPostOption->operation_timeout;
        }
    }
}

aos_status_t* log_post_logs_from_http_cont_with_option(log_http_cont* cont, log_post_option * logPostOption){
    
    aos_http_request_t *req = (aos_http_request_t*)apr_palloc(cont->root, sizeof(aos_http_request_t));
    req->pool = cont->root;
    req->headers = cont->headers;
    req->body_len = cont->body.length;
    req->method = HTTP_POST;
    req->signed_url = cont->url;
    req->file_path = NULL;
    req->file_buf = NULL;
    
    log_request_options_t *options = log_request_options_create(cont->root);
    
    options->ctl = aos_http_controller_create(cont->root, 0);
    options->ctl->pool = cont->root;
    _copy_option(logPostOption, options);
    aos_list_t buffer;
    aos_list_init(&buffer);
    
    aos_buf_t *content = aos_buf_pack(cont->root, cont->body.data, (int)cont->body.length);
    aos_list_add_tail(&content->node, &buffer);
    log_write_request_body_from_buffer(&buffer, req);
    
    aos_http_response_t *resp = aos_http_response_create(options->pool);
    
    req->read_body = aos_read_http_body_memory;
    aos_status_t* s = log_send_request(options->ctl, req, resp);
    
    return s;
}

aos_status_t *log_post_logs_from_proto_buf_with_option(const char *endpoint, const char * accesskeyId, const char *accessKey, const char *stsToken, const char *project, const char *logstore, log_group_builder* bder, log_post_option * logPostOption)
{
    aos_pool_t *p = bder->root;
    aos_string_t project_name, logstore_name;
    aos_table_t *headers = NULL;
    aos_table_t *resp_headers = NULL;
    log_request_options_t *options = NULL;
    aos_list_t buffer;
    unsigned char *md5 = NULL;
    char *buf = NULL;
    int64_t buf_len;
    char *b64_value = NULL;
    aos_buf_t *content = NULL;
    aos_status_t *s = NULL;
    
    options = log_request_options_create(p);
    options->config = log_config_create(options->pool);
    aos_str_set(&(options->config->endpoint), endpoint);
    aos_str_set(&(options->config->access_key_id), accesskeyId);
    aos_str_set(&(options->config->access_key_secret), accessKey);
    if(stsToken != NULL)
    {
        aos_str_set(&(options->config->sts_token), stsToken);
    }
    options->ctl = aos_http_controller_create(options->pool, 0);
    _copy_option(logPostOption, options);
    headers = aos_table_make(p, 5);
    apr_table_set(headers, LOG_API_VERSION, "0.6.0");
    apr_table_set(headers, LOG_COMPRESS_TYPE, "lz4");
    apr_table_set(headers, LOG_SIGNATURE_METHOD, "hmac-sha1");
    apr_table_set(headers, LOG_CONTENT_TYPE, "application/x-protobuf");
    aos_str_set(&project_name, project);
    aos_str_set(&logstore_name, logstore);
    
    aos_list_init(&buffer);
    
    log_buf* buff = serialize_to_proto_buf(bder);
    
    char *body = buff->data;
    
    if(body == NULL)
    {
        s = aos_status_create(options->pool);
        aos_status_set(s, 400, AOS_CLIENT_ERROR_CODE, "fail to format proto buffer data");
        return s;
    }
    int org_body_size = (int)buff->length;
    apr_table_set(headers, LOG_BODY_RAW_SIZE, apr_itoa(options->pool, org_body_size));
    int compress_bound = LZ4_compressBound(org_body_size);
    char *compress_data = aos_pcalloc(options->pool, compress_bound);
    int compressed_size = LZ4_compress(body, compress_data, org_body_size);
    if(compressed_size <= 0)
    {
        s = aos_status_create(options->pool);
        aos_status_set(s, 400, AOS_CLIENT_ERROR_CODE, "fail to compress json data");
        return s;
    }
    content = aos_buf_pack(options->pool, compress_data, compressed_size);
    aos_list_add_tail(&content->node, &buffer);
    
    //add Content-MD5
    buf_len = aos_buf_list_len(&buffer);
    buf = aos_buf_list_content(options->pool, &buffer);
    md5 = aos_md5(options->pool, buf, (apr_size_t)buf_len);
    b64_value = aos_pcalloc(options->pool, 50);
    int loop = 0;
    for(; loop < 16; ++loop)
    {
        unsigned char a = ((*md5)>>4) & 0xF, b = (*md5) & 0xF;
        b64_value[loop<<1] = a > 9 ? (a - 10 + 'A') : (a + '0');
        b64_value[(loop<<1)|1] = b > 9 ? (b - 10 + 'A') : (b + '0');
        ++md5;
    }
    b64_value[loop<<1] = '\0';
    apr_table_set(headers, LOG_CONTENT_MD5, b64_value);
    
    s = log_post_logs_from_buffer(options, &project_name, &logstore_name,
                                  &buffer, headers, &resp_headers);
    return s;
    
}

aos_status_t *log_post_logs_with_sts_token_with_option(aos_pool_t *p, const char *endpoint, const char * accesskeyId, const char *accessKey, const char *stsToken, const char *project, const char *logstore, cJSON *root, log_post_option * logPostOption)
{
    aos_string_t project_name, logstore_name;
    aos_table_t *headers = NULL;
    aos_table_t *resp_headers = NULL;
    log_request_options_t *options = NULL;
    aos_list_t buffer;
    unsigned char *md5 = NULL;
    char *buf = NULL;
    int64_t buf_len;
    char *b64_value = NULL;
    aos_buf_t *content = NULL;
    aos_status_t *s = NULL;

    options = log_request_options_create(p);
    options->config = log_config_create(options->pool);
    aos_str_set(&(options->config->endpoint), endpoint);
    aos_str_set(&(options->config->access_key_id), accesskeyId);
    aos_str_set(&(options->config->access_key_secret), accessKey);
    if(stsToken != NULL)
    {
        aos_str_set(&(options->config->sts_token), stsToken);
    }
    options->ctl = aos_http_controller_create(options->pool, 0);
    _copy_option(logPostOption, options);
    headers = aos_table_make(p, 5);
    apr_table_set(headers, LOG_API_VERSION, "0.6.0");
    apr_table_set(headers, LOG_COMPRESS_TYPE, "lz4");
    apr_table_set(headers, LOG_SIGNATURE_METHOD, "hmac-sha1");
    apr_table_set(headers, LOG_CONTENT_TYPE, "application/json");
    aos_str_set(&project_name, project);
    aos_str_set(&logstore_name, logstore);

    aos_list_init(&buffer);
    char *body = cJSON_PrintUnformatted(root);
    if(body == NULL)
    {
        s = aos_status_create(options->pool);
        aos_status_set(s, 400, AOS_CLIENT_ERROR_CODE, "fail to format cJSON data");
        return s;
    }
    int org_body_size = strlen(body);
    apr_table_set(headers, LOG_BODY_RAW_SIZE, apr_itoa(options->pool, org_body_size));
    int compress_bound = LZ4_compressBound(org_body_size);
    char *compress_data = aos_pcalloc(options->pool, compress_bound);
    int compressed_size = LZ4_compress(body, compress_data, org_body_size);
    if(compressed_size <= 0)
    {
        s = aos_status_create(options->pool);
        aos_status_set(s, 400, AOS_CLIENT_ERROR_CODE, "fail to compress json data");
        return s;
    }
    content = aos_buf_pack(options->pool, compress_data, compressed_size);
    aos_list_add_tail(&content->node, &buffer);

    //add Content-MD5
    buf_len = aos_buf_list_len(&buffer);
    buf = aos_buf_list_content(options->pool, &buffer);
    md5 = aos_md5(options->pool, buf, (apr_size_t)buf_len);
    b64_value = aos_pcalloc(options->pool, 50);
    int loop = 0;
    for(; loop < 16; ++loop)
    {
        unsigned char a = ((*md5)>>4) & 0xF, b = (*md5) & 0xF;
        b64_value[loop<<1] = a > 9 ? (a - 10 + 'A') : (a + '0');
        b64_value[(loop<<1)|1] = b > 9 ? (b - 10 + 'A') : (b + '0');
        ++md5;
    }
    b64_value[loop<<1] = '\0';
    apr_table_set(headers, LOG_CONTENT_MD5, b64_value);

    s = log_post_logs_from_buffer(options, &project_name, &logstore_name,
				   &buffer, headers, &resp_headers);
    free(body);
    return s;
}

aos_status_t *log_post_logs_with_option(aos_pool_t *p, const char *endpoint, const char * accesskeyId, const char *accessKey, const char *project, const char *logstore, cJSON *root, log_post_option * logPostOption)
{
    return log_post_logs_with_sts_token_with_option(p, endpoint, accesskeyId, accessKey, NULL, project, logstore, root, logPostOption);
}




aos_status_t *log_post_logs_with_sts_token(aos_pool_t *p, const char *endpoint, const char * accesskeyId, const char *accessKey, const char *stsToken, const char *project, const char *logstore, cJSON *root)
{
    return log_post_logs_with_sts_token_with_option(p, endpoint, accesskeyId, accessKey, stsToken, project, logstore, root, NULL);

}

aos_status_t *log_post_logs_from_proto_buf(const char *endpoint, const char * accesskeyId, const char *accessKey, const char *stsToken, const char *project, const char *logstore, log_group_builder* bder)
{
    return log_post_logs_from_proto_buf_with_option(endpoint, accesskeyId, accessKey, stsToken, project, logstore, bder, NULL);
}

aos_status_t* log_post_logs_from_http_cont(log_http_cont* cont)
{
    return log_post_logs_from_http_cont_with_option(cont, NULL);
}

aos_status_t *log_post_logs(aos_pool_t *p, const char *endpoint, const char * accesskeyId, const char *accessKey, const char *project, const char *logstore, cJSON *root)
{
    return log_post_logs_with_option(p, endpoint, accesskeyId, accessKey, project, logstore, root, NULL);
}
