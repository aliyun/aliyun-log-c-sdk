#include "aos_log.h"
#include "aos_util.h"
#include "aos_string.h"
#include "aos_http_io.h"
#include "aos_transport.h"

static int aos_curl_code_to_status(CURLcode code);
static void aos_init_curl_headers(aos_curl_http_transport_t *t);
static void aos_transport_cleanup(aos_http_transport_t *t);
static int aos_init_curl_url(aos_curl_http_transport_t *t);
static void aos_curl_transport_headers_done(aos_curl_http_transport_t *t);
static int aos_curl_transport_setup(aos_curl_http_transport_t *t);
static void aos_curl_transport_finish(aos_curl_http_transport_t *t);
static void aos_move_transport_state(aos_curl_http_transport_t *t, aos_transport_state_e s);

static size_t aos_curl_default_header_callback(char *buffer, size_t size, size_t nitems, void *userdata);
static size_t aos_curl_default_write_callback(char *ptr, size_t size, size_t nmemb, void *userdata);
static size_t aos_curl_default_read_callback(char *buffer, size_t size, size_t nitems, void *instream);

static void aos_init_curl_headers(aos_curl_http_transport_t *t)
{
    int pos;
    char *header;
    const aos_array_header_t *tarr;
    const aos_table_entry_t *telts;
    union aos_func_u func;

    if (t->req->method == HTTP_PUT || t->req->method == HTTP_POST) {
        header = apr_psprintf(t->pool, "Content-Length: %" APR_INT64_T_FMT, t->req->body_len);
        t->headers = curl_slist_append(t->headers, header);
    }

    tarr = aos_table_elts(t->req->headers);
    telts = (aos_table_entry_t*)tarr->elts;
    for (pos = 0; pos < tarr->nelts; ++pos) {
        header = apr_psprintf(t->pool, "%s: %s", telts[pos].key, telts[pos].val);
        t->headers = curl_slist_append(t->headers, header);
    }
    
    func.func1 = (aos_func1_pt)curl_slist_free_all;
    aos_fstack_push(t->cleanup, t->headers, func, 1);
}

static int aos_init_curl_url(aos_curl_http_transport_t *t)
{
    int rs;
    const char *proto;
    aos_string_t querystr;
    char uristr[3*AOS_MAX_URI_LEN+1];

    uristr[0] = '\0';
    aos_str_null(&querystr);
    
    if ((rs = aos_url_encode(uristr, t->req->uri, AOS_MAX_URI_LEN)) != AOSE_OK) {
        t->controller->error_code = rs;
        t->controller->reason = "uri invalid argument.";
        return rs;
    }

    if ((rs = aos_query_params_to_string(t->pool, t->req->query_params, &querystr)) != AOSE_OK) {
        t->controller->error_code = rs;
        t->controller->reason = "query params invalid argument.";
        return rs;
    }

    proto = strlen(t->req->proto) != 0 ? t->req->proto : AOS_HTTP_PREFIX;
    if (querystr.len == 0) {
        t->url = apr_psprintf(t->pool, "%s%s/%s",
                              proto,
                              t->req->host,
                              uristr);
    } else {
        t->url = apr_psprintf(t->pool, "%s%s/%s%.*s",
                              proto,
                              t->req->host,
                              uristr,
                              querystr.len,
                              querystr.data);
    }
    aos_debug_log("url:%s.", t->url);

    return AOSE_OK;
}

static void aos_transport_cleanup(aos_http_transport_t *t)
{
    int s;
    char buf[256];

    if (t->req->file_buf != NULL && t->req->file_buf->owner) {
        aos_trace_log("close request body file.");
        if ((s = apr_file_close(t->req->file_buf->file)) != APR_SUCCESS) {
            aos_warn_log("apr_file_close failure, %s.", apr_strerror(s, buf, sizeof(buf)));
        }
        t->req->file_buf = NULL;
    }
    
    if (t->resp->file_buf != NULL && t->resp->file_buf->owner) {
        aos_trace_log("close response body file.");
        if ((s = apr_file_close(t->resp->file_buf->file)) != APR_SUCCESS) {
            aos_warn_log("apr_file_close failure, %s.", apr_strerror(s, buf, sizeof(buf)));
        }
        t->resp->file_buf = NULL;
    }
}

aos_http_transport_t *aos_curl_http_transport_create(aos_pool_t *p)
{
    aos_func_u func;
    aos_curl_http_transport_t *t;

    t = (aos_curl_http_transport_t *)aos_pcalloc(p, sizeof(aos_curl_http_transport_t));

    t->pool = p;
    t->options = aos_default_http_transport_options;
    t->cleanup = aos_fstack_create(p, 5);

    func.func1 = (aos_func1_pt)aos_transport_cleanup;
    aos_fstack_push(t->cleanup, t, func, 1);
    
    t->curl = aos_request_get();
    func.func1 = (aos_func1_pt)request_release;
    aos_fstack_push(t->cleanup, t->curl, func, 1);

    t->header_callback = aos_curl_default_header_callback;
    t->read_callback = aos_curl_default_read_callback;
    t->write_callback = aos_curl_default_write_callback;

    return (aos_http_transport_t *)t;
}

static void aos_move_transport_state(aos_curl_http_transport_t *t, aos_transport_state_e s)
{
    if (t->state < s) {
        t->state = s;
    }
}

void aos_curl_response_headers_parse(aos_pool_t *p, aos_table_t *headers, char *buffer, int len)
{
    char *pos;
    aos_string_t str;
    aos_string_t key;
    aos_string_t value;
    
    str.data = buffer;
    str.len = len;

    aos_trip_space_and_cntrl(&str);

    pos = aos_strlchr(str.data, str.data + str.len, ':');
    if (pos == NULL) {
        return;
    }
    key.data = str.data;
    key.len = pos - str.data;

    pos += 1;
    value.len = str.data + str.len - pos;
    value.data = pos;
    aos_strip_space(&value);

    apr_table_addn(headers, aos_pstrdup(p, &key), aos_pstrdup(p, &value));
}

size_t aos_curl_default_header_callback(char *buffer, size_t size, size_t nitems, void *userdata)
{
    int len;
    aos_curl_http_transport_t *t;

    t = (aos_curl_http_transport_t *)(userdata);
    len = size * nitems;

    if (t->controller->first_byte_time == 0) {
        t->controller->first_byte_time = apr_time_now();
    }

    aos_curl_response_headers_parse(t->pool, t->resp->headers, buffer, len);

    aos_move_transport_state(t, TRANS_STATE_HEADER);

    return len;
}

static void aos_curl_transport_headers_done(aos_curl_http_transport_t *t)
{
    long http_code;
    CURLcode code;
    const char *value;

    if (t->controller->error_code != AOSE_OK) {
        aos_debug_log("has error %d.", t->controller->error_code);
        return;
    }
    
    if (t->resp->status > 0) {
        aos_trace_log("http response status %d.", t->resp->status);
        return;
    }

    t->resp->status = 0;
    if ((code = curl_easy_getinfo(t->curl, CURLINFO_RESPONSE_CODE, &http_code)) != CURLE_OK) {
        t->controller->reason = apr_pstrdup(t->pool, curl_easy_strerror(code));
        t->controller->error_code = AOSE_INTERNAL_ERROR;
        return;
    } else {
        t->resp->status = http_code;
    }

    value = apr_table_get(t->resp->headers, "Content-Length");
    if (value != NULL) {
        t->resp->content_length = apr_atoi64(value);
    }
}

size_t aos_curl_default_write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    int len;
    int bytes;
    aos_curl_http_transport_t *t;

    t = (aos_curl_http_transport_t *)(userdata);
    len = size * nmemb;

    if (t->controller->first_byte_time == 0) {
        t->controller->first_byte_time = apr_time_now();
    }
    
    aos_curl_transport_headers_done(t);

    if (t->controller->error_code != AOSE_OK) {
        aos_debug_log("write callback abort");
        return 0;
    }

    // On HTTP error, we expect to parse an HTTP error response    
    if (t->resp->status < 200 || t->resp->status > 299) {
        bytes = aos_write_http_body_memory(t->resp, ptr, len);
        assert(bytes == len);
        aos_move_transport_state(t, TRANS_STATE_BODY_IN);
        return bytes;
    }

    if (t->resp->type == BODY_IN_MEMORY && t->resp->body_len >= (int64_t)t->controller->options->max_memory_size) {
        t->controller->reason = apr_psprintf(t->pool,
             "receive body too big, current body size: %" APR_INT64_T_FMT ", max memory size: %" APR_INT64_T_FMT,
              t->resp->body_len, t->controller->options->max_memory_size);
        t->controller->error_code = AOSE_OVER_MEMORY;
        aos_error_log("error reason:%s, ", t->controller->reason);
        return 0;
    }

    if ((bytes = t->resp->write_body(t->resp, ptr, len)) < 0) {
        aos_debug_log("write body failure, %d.", bytes);
        t->controller->error_code = AOSE_WRITE_BODY_ERROR;
        t->controller->reason = "write body failure.";
        return 0;
    }
    
    aos_move_transport_state(t, TRANS_STATE_BODY_IN);
    
    return bytes;
}

size_t aos_curl_default_read_callback(char *buffer, size_t size, size_t nitems, void *instream)
{
    int len;
    int bytes;
    aos_curl_http_transport_t *t;
    
    t = (aos_curl_http_transport_t *)(instream);
    len = size * nitems;

    if (t->controller->error_code != AOSE_OK) {
        aos_debug_log("abort read callback.");
        return CURL_READFUNC_ABORT;
    }

    if ((bytes = t->req->read_body(t->req, buffer, len)) < 0) {
        aos_debug_log("read body failure, %d.", bytes);
        t->controller->error_code = AOSE_READ_BODY_ERROR;
        t->controller->reason = "read body failure.";
        return CURL_READFUNC_ABORT;
    }
    
    aos_move_transport_state(t, TRANS_STATE_BODY_OUT);

    return bytes;
}

static int aos_curl_code_to_status(CURLcode code)
{
    switch (code) {
        case CURLE_OUT_OF_MEMORY:
            return AOSE_OUT_MEMORY;
        case CURLE_COULDNT_RESOLVE_PROXY:
        case CURLE_COULDNT_RESOLVE_HOST:
            return AOSE_NAME_LOOKUP_ERROR;
        case CURLE_COULDNT_CONNECT:
            return AOSE_FAILED_CONNECT;
        case CURLE_WRITE_ERROR:
        case CURLE_OPERATION_TIMEDOUT:
            return AOSE_CONNECTION_FAILED;
        case CURLE_PARTIAL_FILE:
            return AOSE_OK;
        case CURLE_SSL_CACERT:
            return AOSE_FAILED_VERIFICATION;
        default:
            return AOSE_INTERNAL_ERROR;
    }
}

static void aos_curl_transport_finish(aos_curl_http_transport_t *t)
{
    aos_curl_transport_headers_done(t);
    
    if (t->cleanup != NULL) {
        aos_fstack_destory(t->cleanup);
        t->cleanup = NULL;
    }
}

int aos_curl_transport_setup(aos_curl_http_transport_t *t)
{
    CURLcode code;

#define curl_easy_setopt_safe(opt, val)                                 \
    if ((code = curl_easy_setopt(t->curl, opt, val)) != CURLE_OK) {    \
            t->controller->reason = apr_pstrdup(t->pool, curl_easy_strerror(code)); \
            t->controller->error_code = AOSE_FAILED_INITIALIZE;         \
            aos_error_log("curl_easy_setopt failed, code:%d %s.", code, t->controller->reason); \
            return AOSE_FAILED_INITIALIZE;                              \
    }

    curl_easy_setopt_safe(CURLOPT_PRIVATE, t);

    curl_easy_setopt_safe(CURLOPT_HEADERDATA, t);
    curl_easy_setopt_safe(CURLOPT_HEADERFUNCTION, t->header_callback);
    
    curl_easy_setopt_safe(CURLOPT_READDATA, t);
    curl_easy_setopt_safe(CURLOPT_READFUNCTION, t->read_callback);
    
    curl_easy_setopt_safe(CURLOPT_WRITEDATA, t);    
    curl_easy_setopt_safe(CURLOPT_WRITEFUNCTION, t->write_callback);

    curl_easy_setopt_safe(CURLOPT_FILETIME, 1);
    curl_easy_setopt_safe(CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt_safe(CURLOPT_NOPROGRESS, 1);
    curl_easy_setopt_safe(CURLOPT_TCP_NODELAY, 1);
    curl_easy_setopt_safe(CURLOPT_NETRC, CURL_NETRC_IGNORED);

    // transport options
    curl_easy_setopt_safe(CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt_safe(CURLOPT_USERAGENT, t->options->user_agent);

    // request options
    curl_easy_setopt_safe(CURLOPT_DNS_CACHE_TIMEOUT, t->controller->options->dns_cache_timeout);
    curl_easy_setopt_safe(CURLOPT_CONNECTTIMEOUT, t->controller->options->connect_timeout);
    curl_easy_setopt_safe(CURLOPT_LOW_SPEED_LIMIT, t->controller->options->speed_limit);
    curl_easy_setopt_safe(CURLOPT_LOW_SPEED_TIME, t->controller->options->speed_time);

    aos_init_curl_headers(t);
    curl_easy_setopt_safe(CURLOPT_HTTPHEADER, t->headers);

    if (NULL == t->req->signed_url) {
        if (aos_init_curl_url(t) != AOSE_OK) {
            return t->controller->error_code;
        }
    }
    else {
        t->url = t->req->signed_url; 
    }
    curl_easy_setopt_safe(CURLOPT_URL, t->url);

    switch (t->req->method) {
        case HTTP_HEAD:
            curl_easy_setopt_safe(CURLOPT_NOBODY, 1);
            break;
        case HTTP_PUT:
            curl_easy_setopt_safe(CURLOPT_UPLOAD, 1);
            break;
        case HTTP_POST:
            curl_easy_setopt_safe(CURLOPT_POST, 1);
            break;
        case HTTP_DELETE:
            curl_easy_setopt_safe(CURLOPT_CUSTOMREQUEST, "DELETE");
            break;
        default: // HTTP_GET
            break;
    }
    
#undef curl_easy_setopt_safe
    
    t->state = TRANS_STATE_INIT;
    
    return AOSE_OK;
}

int aos_curl_http_transport_perform(aos_http_transport_t *t_)
{
    int ecode;
    CURLcode code;
    aos_curl_http_transport_t *t = (aos_curl_http_transport_t *)(t_);
    ecode = aos_curl_transport_setup(t);
    if (ecode != AOSE_OK) {
        return ecode;
    }

    t->controller->start_time = apr_time_now();
    code = curl_easy_perform(t->curl);
    t->controller->finish_time = apr_time_now();
    aos_move_transport_state(t, TRANS_STATE_DONE);
    
    if ((code != CURLE_OK) && (t->controller->error_code == AOSE_OK)) {
        ecode = aos_curl_code_to_status(code);
        if (ecode != AOSE_OK) {
            t->controller->error_code = ecode;
            t->controller->reason = apr_pstrdup(t->pool, curl_easy_strerror(code));
            aos_error_log("transport failure curl code:%d error:%s", code, t->controller->reason);
        }
    }
    
    aos_curl_transport_finish(t);
    
    return t->controller->error_code;
}
