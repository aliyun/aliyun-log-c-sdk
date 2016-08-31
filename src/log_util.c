#include <arpa/inet.h>
#include "aos_log.h"
#include "aos_util.h"
#include "aos_string.h"
#include "aos_status.h"
#include "log_util.h"
#include "log_auth.h"

log_request_options_t *log_request_options_create(aos_pool_t *p)
{
    int s;
    log_request_options_t *options;

    if(p == NULL) {
        if ((s = aos_pool_create(&p, NULL)) != APR_SUCCESS) {
            aos_fatal_log("aos_pool_create failure.");
            return NULL;
        }
    }

    options = (log_request_options_t *)aos_pcalloc(p, sizeof(log_request_options_t));
    options->pool = p;

    return options;
}

int starts_with(const aos_string_t *str, const char *prefix) {
    uint32_t i;
    if(NULL != str && prefix && str->len > 0 && strlen(prefix)) {
        for(i = 0; str->data[i] != '\0' && prefix[i] != '\0'; i++) {
            if(prefix[i] != str->data[i]) return 0;
        }
        return 1;
    }
    return 0;
}

void generate_proto(const log_request_options_t *options, aos_http_request_t *req)
{
    const char *proto;
    proto = starts_with(&options->config->endpoint, AOS_HTTP_PREFIX) ? AOS_HTTP_PREFIX : "";
    proto = starts_with(&options->config->endpoint, AOS_HTTPS_PREFIX) ? AOS_HTTPS_PREFIX : proto;
    req->proto = apr_psprintf(options->pool, "%.*s", (int)strlen(proto), proto);
}

log_config_t *log_config_create(aos_pool_t *p)
{
    return (log_config_t *)aos_pcalloc(p, sizeof(log_config_t));
}

void log_post_logs_uri(const log_request_options_t *options,
        const aos_string_t *project,
        const aos_string_t *logstore,
        aos_http_request_t *req)
{
    generate_proto(options, req);

    int32_t proto_len = strlen(req->proto);

    req->resource = apr_psprintf(options->pool, "logstores/%.*s/shards/lb",
            logstore->len, logstore->data);

    aos_string_t raw_endpoint = {options->config->endpoint.len - proto_len,
        options->config->endpoint.data + proto_len};

    req->host = apr_psprintf(options->pool, "%.*s.%.*s",
            project->len, project->data, 
            raw_endpoint.len, raw_endpoint.data);
    
    //req->uri = apr_psprintf(options->pool, "%.*s%.*s/logstores/%.*s/shards/lb", (int)strlen(req->proto),req->proto,(int)strlen(req->host), req->host,logstore->len, logstore->data);
    
    req->uri = apr_psprintf(options->pool, "logstores/%.*s/shards/lb", logstore->len, logstore->data);
}


void log_init_request(const log_request_options_t *options, 
        http_method_e method,
        aos_http_request_t **req, 
        aos_table_t *params, 
        aos_table_t *headers, 
        aos_http_response_t **resp)
{
    *req = aos_http_request_create(options->pool);
    *resp = aos_http_response_create(options->pool);
    (*req)->method = method;
    if (options->config->sts_token.data != NULL)
    {
        apr_table_set(headers, ACS_SECURITY_TOKEN, options->config->sts_token.data);
    }
    (*req)->headers = headers;
    (*req)->query_params = params;
}

void log_post_logs_request(const log_request_options_t *options, 
        const aos_string_t *project,
        const aos_string_t *logstore, 
        http_method_e method, 
        aos_http_request_t **req, 
        aos_table_t *params, 
        aos_table_t *headers,
        aos_http_response_t **resp)
{
    log_init_request(options, method, req, params, headers, resp);
    log_post_logs_uri(options, project, logstore, *req);
}
aos_status_t *log_send_request(aos_http_controller_t *ctl, 
        aos_http_request_t *req,
        aos_http_response_t *resp)
{
    aos_status_t *s;
    const char *reason;
    int res = AOSE_OK;

    s = aos_status_create(ctl->pool);
    res = aos_http_send_request(ctl, req, resp);

    if (res != AOSE_OK) {
        reason = aos_http_controller_get_reason(ctl);
        aos_status_set(s, res, AOS_HTTP_IO_ERROR_CODE, reason);
    } else if (!aos_http_is_ok(resp->status)) {
        s = aos_status_parse_from_body(ctl->pool, &resp->body, resp->status, s);
    } else {
        s->code = resp->status;
    }

    s->req_id = (char*)(apr_table_get(resp->headers, LOG_REQUEST_ID));
    if (s->req_id == NULL) {
        s->req_id = "";
    }

    return s;
}
aos_status_t *log_process_request(const log_request_options_t *options,
        aos_http_request_t *req, 
        aos_http_response_t *resp)
{
    int res = AOSE_OK;
    aos_status_t *s;

    s = aos_status_create(options->pool);
    res = log_sign_request(req, options->config);
    if (res != AOSE_OK) {
        aos_status_set(s, res, AOS_CLIENT_ERROR_CODE, NULL);
        return s;
    }

    return log_send_request(options->ctl, req, resp);
}
void log_write_request_body_from_buffer(aos_list_t *buffer, 
        aos_http_request_t *req)
{
    aos_list_movelist(buffer, &req->body);
    req->body_len = aos_buf_list_len(&req->body);
}
aos_table_t* aos_table_create_if_null(const log_request_options_t *options, 
        aos_table_t *table, 
        int table_size) 
{
    if (table == NULL) {
        table = aos_table_make(options->pool, table_size);
    }   
    return table;
}
aos_status_t *log_post_logs_from_buffer(const log_request_options_t *options,
        const aos_string_t *project,
        const aos_string_t *logstore, 
        aos_list_t *buffer,
        aos_table_t *headers, 
        aos_table_t **resp_headers)
{
    aos_status_t *s = NULL;
    aos_http_request_t *req = NULL;
    aos_http_response_t *resp = NULL;
    aos_table_t *query_params = NULL;

    headers = aos_table_create_if_null(options, headers, 0);

    query_params = aos_table_create_if_null(options, query_params, 0);

    log_post_logs_request(options, project, logstore, HTTP_POST, 
            &req, query_params, headers, &resp);
    log_write_request_body_from_buffer(buffer, req);

    s = log_process_request(options, req, resp);
    *resp_headers = resp->headers;

    return s;
}

