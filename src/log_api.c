#include "lz4.h"
#include "aos_log.h"
#include "aos_util.h"
#include "aos_string.h"
#include "aos_status.h"
#include "log_auth.h"
#include "log_util.h"
#include "log_api.h"

log_http_cont* log_get_http_cont(const char *endpoint, const char * accesskeyId, const char *accessKey, const char *stsToken,const char* project,const char* logstore,log_group_builder* bder)
{
    aos_pool_t* tmp;
    apr_pool_create(&tmp, NULL);
    aos_http_request_t *req = (aos_http_request_t*)apr_palloc(tmp, sizeof(aos_http_request_t));
    req->pool = tmp;
    
    aos_string_t project_name, logstore_name;
    aos_table_t *headers = NULL;
    log_request_options_t *options = NULL;
    aos_list_t buffer;
    unsigned char *md5 = NULL;
    char *buf = NULL;
    int64_t buf_len;
    char *b64_value = NULL;
    aos_buf_t *content = NULL;
    aos_status_t *s = NULL;
    
    options = log_request_options_create(tmp);
    options->config = log_config_create(tmp);
    aos_str_set(&(options->config->endpoint), endpoint);
    aos_str_set(&(options->config->access_key_id), accesskeyId);
    aos_str_set(&(options->config->access_key_secret), accessKey);
    if(stsToken != NULL)
    {
        aos_str_set(&(options->config->sts_token), stsToken);
    }
    options->ctl = aos_http_controller_create(tmp, 0);
    headers = aos_table_make(options->pool, 5);
    apr_table_set(headers, LOG_API_VERSION, "0.6.0");
    apr_table_set(headers, LOG_COMPRESS_TYPE, "lz4");
    apr_table_set(headers, LOG_SIGNATURE_METHOD, "hmac-sha1");
    apr_table_set(headers, LOG_CONTENT_TYPE, "application/x-protobuf");
    aos_str_set(&project_name, project);
    aos_str_set(&logstore_name, logstore);
    
    aos_list_init(&buffer);
    
    log_buf* buff = serialize_to_proto_buf(bder);
    char *body = buff->data;
    int org_body_size = (int)buff->length;
    apr_table_set(headers, LOG_BODY_RAW_SIZE, apr_itoa(options->pool, org_body_size));
    int compress_bound = LZ4_compressBound(org_body_size);
    char *compress_data = aos_pcalloc(tmp, compress_bound);
    int compressed_size = LZ4_compress(body, compress_data, org_body_size);
    content = aos_buf_pack(tmp, compress_data, compressed_size);
    aos_list_add_tail(&content->node, &buffer);
    
    //add Content-MD5
    buf_len = aos_buf_list_len(&buffer);
    buf = aos_buf_list_content(tmp, &buffer);
    md5 = aos_md5(tmp, buf, (apr_size_t)buf_len);
    b64_value = aos_pcalloc(tmp, 50);
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
   
    aos_http_response_t *resp = NULL;
    aos_table_t *query_params = NULL;
    headers = aos_table_create_if_null(options, headers, 0);
    
    query_params = aos_table_create_if_null(options, query_params, 0);
    
    log_post_logs_request(options, &project_name, &logstore_name, HTTP_POST,
                          &req, query_params, headers, &resp);
    log_write_request_body_from_buffer(&buffer, req);
    
    int res = AOSE_OK;
    s = aos_status_create(options->pool);
    res = log_sign_request(req, options->config);

    log_http_cont* cont_out = (log_http_cont*)apr_palloc(req->pool, sizeof(log_http_cont));
    cont_out->body.data = compress_data;
    cont_out->body.length = compressed_size;
    cont_out->root = req->pool;
    cont_out->headers = req->headers;
    cont_out->host = req->host;
    cont_out->proto = req->proto;
    cont_out->uri = req->uri;
    
    return cont_out;
}

void log_clean_http_cont(log_http_cont* cont)
{
    apr_pool_destroy(cont->root);
}

aos_status_t *log_post_logs_from_proto_buf(const char *endpoint, const char * accesskeyId, const char *accessKey, const char *stsToken, const char *project, const char *logstore, log_group_builder* bder)
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


aos_status_t *log_post_logs_with_sts_token(aos_pool_t *p, const char *endpoint, const char * accesskeyId, const char *accessKey, const char *stsToken, const char *project, const char *logstore, cJSON *root)
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
aos_status_t *log_post_logs(aos_pool_t *p, const char *endpoint, const char * accesskeyId, const char *accessKey, const char *project, const char *logstore, cJSON *root)
{
    return log_post_logs_with_sts_token(p, endpoint, accesskeyId, accessKey, NULL, project, logstore, root);
}
