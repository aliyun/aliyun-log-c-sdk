//
//  log_http_cont.c
//  log-c-sdk-new
//
//  Created by 王佳玮 on 16/8/31.
//  Copyright © 2016年 wangjwchn. All rights reserved.
//

#include "log_http_cont.h"
#include "lz4.h"
#include "log_auth.h"
#include <stdlib.h>

void log_clean_http_cont(log_http_cont* cont)
{
    apr_pool_destroy(cont->root);
}

int _starts_with(const aos_string_t *str, const char *prefix) {
    uint32_t i;
    if(NULL != str && prefix && str->len > 0 && strlen(prefix)) {
        for(i = 0; str->data[i] != '\0' && prefix[i] != '\0'; i++) {
            if(prefix[i] != str->data[i]) return 0;
        }
        return 1;
    }
    return 0;
}

log_buf* _compressed_buffer(apr_pool_t* pool,log_buf* before)
{
    char *body = before->data;
    int compress_bound = LZ4_compressBound((int)before->length);
    char *compress_data = aos_pcalloc(pool, compress_bound);
    int compressed_size = LZ4_compress_default(body, compress_data, (int)before->length, compress_bound);
    
    log_buf* after = aos_pcalloc(pool, sizeof(log_buf));
    
    after->data = compress_data;
    after->length = compressed_size;
    return after;
}

char* _get_url(apr_pool_t* pool,aos_string_t _endpoint,aos_string_t logstore_name,aos_string_t project_name)
{
    const char *proto;
    proto = _starts_with(&_endpoint, AOS_HTTP_PREFIX) ? AOS_HTTP_PREFIX : "";
    proto = _starts_with(&_endpoint, AOS_HTTPS_PREFIX) ? AOS_HTTPS_PREFIX : proto;
    int32_t proto_len = (int)strlen(proto);
    aos_string_t raw_endpoint = {_endpoint.len - proto_len,
        _endpoint.data + proto_len};
    char* host = apr_psprintf(pool, "%.*s.%.*s",
                              project_name.len, project_name.data,
                              raw_endpoint.len, raw_endpoint.data);
    char* url = apr_psprintf(pool, "%.*s%.*s/logstores/%.*s/shards/lb", (int)strlen(proto),proto,(int)strlen(host), host,logstore_name.len, logstore_name.data);
    return url;
}
char* _get_md5(apr_pool_t* pool,log_buf* buff)
{
    aos_buf_t* content = aos_buf_pack(pool, buff->data, (int)buff->length);
    aos_list_t buffer;
    aos_list_init(&buffer);
    aos_list_add_tail(&content->node, &buffer);
    
    //add Content-MD5
    int64_t buf_len = aos_buf_list_len(&buffer);
    char* buf = aos_buf_list_content(pool, &buffer);
    unsigned char* md5 = aos_md5(pool, buf, (apr_size_t)buf_len);
    char* b64_value = aos_pcalloc(pool, 50);
    int loop = 0;
    for(; loop < 16; ++loop)
    {
        unsigned char a = ((*md5)>>4) & 0xF, b = (*md5) & 0xF;
        b64_value[loop<<1] = a > 9 ? (a - 10 + 'A') : (a + '0');
        b64_value[(loop<<1)|1] = b > 9 ? (b - 10 + 'A') : (b + '0');
        ++md5;
    }
    b64_value[loop<<1] = '\0';
    return b64_value;
}

void _get_lz4_buf_md5(lz4_log_buf * pLogBuffer, char * b64_value)
{
    unsigned char md5Buf[17];
    apr_md5_ctx_t context;
    if (0 != apr_md5_init(&context)) {
        return;
    }

    if (0 != apr_md5_update(&context, pLogBuffer->data, pLogBuffer->length)) {
        return;
    }

    if (0 != apr_md5_final(md5Buf, &context)) {
        return;
    }
    md5Buf[16] = '\0';

    int loop = 0;
    for(; loop < 16; ++loop)
    {
        unsigned char a = ((md5Buf[loop])>>4) & 0xF, b = (md5Buf[loop]) & 0xF;
        b64_value[loop<<1] = a > 9 ? (a - 10 + 'A') : (a + '0');
        b64_value[(loop<<1)|1] = b > 9 ? (b - 10 + 'A') : (b + '0');
    }
    b64_value[loop<<1] = '\0';
    return ;
}

log_http_cont* log_create_http_cont(const char *endpoint, const char * accesskeyId, const char *accessKey, const char *stsToken,const char* project,const char* logstore,log_group_builder* bder)
{
    aos_string_t project_name;
    aos_string_t logstore_name;
    aos_string_t _endpoint;
    aos_string_t _access_key_id;
    aos_string_t _access_key_secret;
    aos_string_t _sts_token;
    
    aos_table_t *headers = NULL;
    
    aos_table_t *query_params = NULL;
    
    aos_str_set(&(_endpoint), endpoint);
    aos_str_set(&(_access_key_id), accesskeyId);
    aos_str_set(&(_access_key_secret), accessKey);
    
    if(stsToken != NULL)
        aos_str_set(&(_sts_token), stsToken);
    
    
    headers = aos_table_make(bder->root, 5);
    apr_table_set(headers, LOG_API_VERSION, "0.6.0");
    apr_table_set(headers, LOG_COMPRESS_TYPE, "lz4");
    apr_table_set(headers, LOG_SIGNATURE_METHOD, "hmac-sha1");
    apr_table_set(headers, LOG_CONTENT_TYPE, "application/x-protobuf");
    aos_str_set(&project_name, project);
    aos_str_set(&logstore_name, logstore);
    if (stsToken != NULL)
    {
        apr_table_set(headers, ACS_SECURITY_TOKEN, stsToken);
    }
    
    log_buf* buff = serialize_to_proto_buf(bder);
    apr_table_set(headers, LOG_BODY_RAW_SIZE, apr_itoa(bder->root, (int)buff->length));
    buff = _compressed_buffer(bder->root, buff);
    
    char* b64_value = _get_md5(bder->root, buff);
    apr_table_set(headers, LOG_CONTENT_MD5, b64_value);
    char* url = _get_url(bder->root, _endpoint, logstore_name, project_name);

    char *resource = apr_psprintf(bder->root, "logstores/%.*s/shards/lb",(int)logstore_name.len, logstore_name.data);
    aos_string_t canon_res;
    char canon_buf[AOS_MAX_URI_LEN];
    char datestr[AOS_MAX_GMT_TIME_LEN];
    const char *value;
    canon_res.data = canon_buf;
    canon_res.len = apr_snprintf(canon_buf, sizeof(canon_buf), "/%s",resource);
    
    if ((value = apr_table_get(headers, LOG_CANNONICALIZED_HEADER_DATE)) == NULL) {
        aos_get_gmt_str_time(datestr);
        apr_table_set(headers, LOG_DATE, datestr);
    }
    
    aos_string_t signstr;
    log_get_string_to_sign(bder->root, HTTP_POST, &canon_res,
                                 headers, query_params, &signstr);
    log_sign_headers(bder->root, &signstr, &_access_key_id,  &_access_key_secret, headers);

    log_http_cont* cont_out = (log_http_cont*)apr_palloc(bder->root, sizeof(log_http_cont));
    cont_out->body.data = buff->data;
    cont_out->body.length = buff->length;
    cont_out->root = bder->root;
    cont_out->headers = headers;
    cont_out->url = url;
    
    return cont_out;
}

log_http_cont* log_create_http_cont_with_lz4_data(const char *endpoint, const char * accesskeyId, const char *accessKey, const char *stsToken, const char* project, const char* logstore, lz4_log_buf * pLZ4buf, apr_pool_t *root)
{
    aos_string_t project_name;
    aos_string_t logstore_name;
    aos_string_t _endpoint;
    aos_string_t _access_key_id;
    aos_string_t _access_key_secret;
    aos_string_t _sts_token;

    aos_table_t *headers = NULL;

    aos_table_t *query_params = NULL;

    aos_str_set(&(_endpoint), endpoint);
    aos_str_set(&(_access_key_id), accesskeyId);
    aos_str_set(&(_access_key_secret), accessKey);

    if(stsToken != NULL)
    aos_str_set(&(_sts_token), stsToken);


    headers = aos_table_make(root, 5);
    apr_table_set(headers, LOG_API_VERSION, "0.6.0");
    apr_table_set(headers, LOG_COMPRESS_TYPE, "lz4");
    apr_table_set(headers, LOG_SIGNATURE_METHOD, "hmac-sha1");
    apr_table_set(headers, LOG_CONTENT_TYPE, "application/x-protobuf");
    aos_str_set(&project_name, project);
    aos_str_set(&logstore_name, logstore);
    if (stsToken != NULL)
    {
        apr_table_set(headers, ACS_SECURITY_TOKEN, stsToken);
    }
    apr_table_set(headers, LOG_BODY_RAW_SIZE, apr_itoa(root, (int)pLZ4buf->raw_length));

    char b64_value[48];
    _get_lz4_buf_md5(pLZ4buf, b64_value);
    b64_value[47] = '\0';
    apr_table_set(headers, LOG_CONTENT_MD5, b64_value);
    char* url = _get_url(root, _endpoint, logstore_name, project_name);

    char *resource = apr_psprintf(root, "logstores/%.*s/shards/lb",(int)logstore_name.len, logstore_name.data);
    aos_string_t canon_res;
    char canon_buf[AOS_MAX_URI_LEN];
    char datestr[AOS_MAX_GMT_TIME_LEN];
    const char *value;
    canon_res.data = canon_buf;
    canon_res.len = apr_snprintf(canon_buf, sizeof(canon_buf), "/%s",resource);

    if ((value = apr_table_get(headers, LOG_CANNONICALIZED_HEADER_DATE)) == NULL) {
        aos_get_gmt_str_time(datestr);
        apr_table_set(headers, LOG_DATE, datestr);
    }

    aos_string_t signstr;
    log_get_string_to_sign(root, HTTP_POST, &canon_res,
                           headers, query_params, &signstr);
    log_sign_headers(root, &signstr, &_access_key_id,  &_access_key_secret, headers);

    log_http_cont* cont_out = (log_http_cont*)apr_palloc(root, sizeof(log_http_cont));
    cont_out->body.data = pLZ4buf->data;
    cont_out->body.length = pLZ4buf->length;
    cont_out->root = root;
    cont_out->headers = headers;
    cont_out->url = url;

    return cont_out;
}
