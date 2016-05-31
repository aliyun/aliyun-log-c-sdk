#include "aos_log.h"
#include "aos_http_io.h"
#include "aos_define.h"
#include <pthread.h>
#include <apr_file_io.h>

aos_pool_t *aos_global_pool = NULL;
apr_file_t *aos_stderr_file = NULL;

aos_http_request_options_t *aos_default_http_request_options = NULL;
aos_http_transport_options_t *aos_default_http_transport_options = NULL;

aos_http_transport_create_pt aos_http_transport_create = aos_curl_http_transport_create;
aos_http_transport_perform_pt aos_http_transport_perform = aos_curl_http_transport_perform;

static pthread_mutex_t requestStackMutexG;
static CURL *requestStackG[AOS_REQUEST_STACK_SIZE];
static int requestStackCountG;
static char aos_user_agent[256];


static aos_http_transport_options_t *aos_http_transport_options_create(aos_pool_t *p);

CURL *aos_request_get()
{
    CURL *request = NULL;
    // Try to get one from the request stack.  We hold the lock for the shortest time possible here.
    pthread_mutex_lock(&requestStackMutexG);
    if (requestStackCountG > 0) {
        request = requestStackG[--requestStackCountG];
    }
    pthread_mutex_unlock(&requestStackMutexG);

    // If we got one, deinitialize it for re-use
    if (request) {
        curl_easy_reset(request);
    }
    else {
        request = curl_easy_init();
    }

    return request;
}

void request_release(CURL *request)
{
    pthread_mutex_lock(&requestStackMutexG);

    // If the request stack is full, destroy this one
    // else put this one at the front of the request stack; we do this because
    // we want the most-recently-used curl handle to be re-used on the next
    // request, to maximize our chances of re-using a TCP connection before it
    // times out
    if (requestStackCountG == AOS_REQUEST_STACK_SIZE) {
        pthread_mutex_unlock(&requestStackMutexG);
        curl_easy_cleanup(request);
    }
    else {
        requestStackG[requestStackCountG++] = request;
        pthread_mutex_unlock(&requestStackMutexG);
    }
}

void aos_set_default_request_options(aos_http_request_options_t *op)
{
    aos_default_http_request_options = op;
}

void aos_set_default_transport_options(aos_http_transport_options_t *op)
{
    aos_default_http_transport_options = op;
}

aos_http_request_options_t *aos_http_request_options_create(aos_pool_t *p)
{
    aos_http_request_options_t *options;
    
    options = (aos_http_request_options_t *)aos_pcalloc(p, sizeof(aos_http_request_options_t));
    options->speed_limit = AOS_MIN_SPEED_LIMIT;
    options->speed_time = AOS_MIN_SPEED_TIME;
    options->connect_timeout = AOS_CONNECT_TIMEOUT;
    options->dns_cache_timeout = AOS_DNS_CACHE_TIMOUT;
    options->max_memory_size = AOS_MAX_MEMORY_SIZE;

    return options;
}

aos_http_transport_options_t *aos_http_transport_options_create(aos_pool_t *p)
{
    return (aos_http_transport_options_t *)aos_pcalloc(p, sizeof(aos_http_transport_options_t));
}

aos_http_controller_t *aos_http_controller_create(aos_pool_t *p, int owner)
{
    int s;
    aos_http_controller_t *ctl;

    if(p == NULL) {
        if ((s = aos_pool_create(&p, NULL)) != APR_SUCCESS) {
            aos_fatal_log("aos_pool_create failure.");
            return NULL;
        }
    }
    ctl = (aos_http_controller_t *)aos_pcalloc(p, sizeof(aos_http_controller_ex_t));
    ctl->pool = p;
    ctl->owner = owner;
    ctl->options = aos_default_http_request_options;

    return ctl;
}

aos_http_request_t *aos_http_request_create(aos_pool_t *p)
{
    aos_http_request_t *req;

    req = (aos_http_request_t *)aos_pcalloc(p, sizeof(aos_http_request_t));
    req->method = HTTP_GET;
    req->headers = aos_table_make(p, 5);
    req->query_params = aos_table_make(p, 3);
    aos_list_init(&req->body);
    req->type = BODY_IN_MEMORY;
    req->body_len = 0;
    req->pool = p;
    req->read_body = aos_read_http_body_memory;

    return req;
}

aos_http_response_t *aos_http_response_create(aos_pool_t *p)
{
    aos_http_response_t *resp;

    resp = (aos_http_response_t *)aos_pcalloc(p, sizeof(aos_http_response_t));
    resp->status = -1;
    resp->headers = aos_table_make(p, 10);
    aos_list_init(&resp->body);
    resp->type = BODY_IN_MEMORY;
    resp->body_len = 0;
    resp->pool = p;
    resp->write_body = aos_write_http_body_memory;

    return resp;
}

int aos_read_http_body_memory(aos_http_request_t *req, char *buffer, int len)
{
    int wsize;
    int bytes = 0;
    aos_buf_t *b;
    aos_buf_t *n;
    
    aos_list_for_each_entry_safe(b, n, &req->body, node) {
        wsize = aos_buf_size(b);
        if (wsize == 0) {
            aos_list_del(&b->node);
            continue;
        }
        wsize = aos_min(len - bytes, wsize);
        if (wsize == 0) {
            break;
        }
        memcpy(buffer + bytes, b->pos, wsize);
        b->pos += wsize;
        bytes += wsize;
        if (b->pos == b->last) {
            aos_list_del(&b->node);
        }
    }

    return bytes;
}

int aos_read_http_body_file(aos_http_request_t *req, char *buffer, int len)
{
    int s;
    char buf[256];
    apr_size_t nbytes = len;
    apr_size_t bytes_left;
    
    if (req->file_buf == NULL || req->file_buf->file == NULL) {
        aos_error_log("request body arg invalid file_buf NULL.");
        return AOSE_INVALID_ARGUMENT;
    }

    if (req->file_buf->file_pos >= req->file_buf->file_last) {
        aos_debug_log("file read finish.");
        return 0;
    }

    bytes_left = req->file_buf->file_last - req->file_buf->file_pos;
    if (nbytes > bytes_left) {
        nbytes = bytes_left;
    }

    if ((s = apr_file_read(req->file_buf->file, buffer, &nbytes)) != APR_SUCCESS) {
        aos_error_log("apr_file_read filure, code:%d %s.", s, apr_strerror(s, buf, sizeof(buf)));
        return AOSE_FILE_READ_ERROR;
    }
    req->file_buf->file_pos += nbytes;
    return nbytes;
}

int aos_write_http_body_memory(aos_http_response_t *resp, const char *buffer, int len)
{
    aos_buf_t *b;

    b = aos_create_buf(resp->pool, len);
    memcpy(b->pos, buffer, len);
    b->last += len;
    aos_list_add_tail(&b->node, &resp->body);
    resp->body_len += len;

    return len;
}

int aos_write_http_body_file(aos_http_response_t *resp, const char *buffer, int len)
{
    int elen;
    int s;
    char buf[256];
    apr_size_t nbytes = len;
    
    if (resp->file_buf == NULL) {
        resp->file_buf = aos_create_file_buf(resp->pool);
    }
    
    if (resp->file_buf->file == NULL) {
        if (resp->file_path == NULL) {
            aos_error_log("resp body file arg NULL.");
            return AOSE_INVALID_ARGUMENT;
        }
        aos_trace_log("open file %s.", resp->file_path);
        if ((elen = aos_open_file_for_write(resp->pool, resp->file_path, resp->file_buf)) != AOSE_OK) {
            return elen;
        }
    }

    assert(resp->file_buf->file != NULL);
    if ((s = apr_file_write(resp->file_buf->file, buffer, &nbytes)) != APR_SUCCESS) {
        aos_error_log("apr_file_write fialure, code:%d %s.", s, apr_strerror(s, buf, sizeof(buf)));
        return AOSE_FILE_WRITE_ERROR;
    }
    
    resp->file_buf->file_last += nbytes;
    resp->body_len += nbytes;

    return nbytes;
}

int aos_http_io_initialize(const char *user_agent_info, int flags)
{
    CURLcode ecode;
    int s;
    char buf[256];
    aos_http_request_options_t *req_options;
    aos_http_transport_options_t *trans_options;

    pthread_mutex_init(&requestStackMutexG, 0);
    requestStackCountG = 0;

    if ((ecode = curl_global_init(CURL_GLOBAL_ALL &
           ~((flags & AOS_INIT_WINSOCK) ? 0: CURL_GLOBAL_WIN32))) != CURLE_OK) 
    {
        aos_error_log("curl_global_init failure, code:%d %s.\n", ecode, curl_easy_strerror(ecode));
        return AOSE_INTERNAL_ERROR;
    }

    if ((s = apr_initialize()) != APR_SUCCESS) {
        aos_error_log("apr_initialize failue.\n");
        return AOSE_INTERNAL_ERROR;
    }

    if (!user_agent_info || !*user_agent_info) {
        user_agent_info = "Unknown";
    }

    if ((s = aos_pool_create(&aos_global_pool, NULL)) != APR_SUCCESS) {
        aos_error_log("aos_pool_create failure, code:%d %s.\n", s, apr_strerror(s, buf, sizeof(buf)));
        return AOSE_INTERNAL_ERROR;
    }
    apr_snprintf(aos_user_agent, sizeof(aos_user_agent)-1, "%s(Compatible %s)", 
                 AOS_VER, user_agent_info);

    if ((s = apr_file_open_stderr(&aos_stderr_file, aos_global_pool)) != APR_SUCCESS) {
        aos_error_log("apr_file_open_stderr failure, code:%d %s.\n", s, apr_strerror(s, buf, sizeof(buf)));
        return AOSE_INTERNAL_ERROR;
    }

    req_options = aos_http_request_options_create(aos_global_pool);
    trans_options = aos_http_transport_options_create(aos_global_pool);
    trans_options->user_agent = aos_user_agent;

    aos_set_default_request_options(req_options);
    aos_set_default_transport_options(trans_options);

    return AOSE_OK;
}

void aos_http_io_deinitialize()
{
    pthread_mutex_destroy(&requestStackMutexG);
    while (requestStackCountG--) {
        curl_easy_cleanup(requestStackG[requestStackCountG]);
    }

    if (aos_stderr_file != NULL) {
        apr_file_close(aos_stderr_file);
        aos_stderr_file = NULL;
    }
    if (aos_global_pool != NULL) {
        aos_pool_destroy(aos_global_pool);
        aos_global_pool = NULL;
    }
    apr_terminate();
}

int aos_http_send_request(aos_http_controller_t *ctl, aos_http_request_t *req, aos_http_response_t *resp)
{
    aos_http_transport_t *t;

    t = aos_http_transport_create(ctl->pool);
    t->req = req;
    t->resp = resp;
    t->controller = (aos_http_controller_ex_t *)ctl;
    
    return aos_http_transport_perform(t);
}

