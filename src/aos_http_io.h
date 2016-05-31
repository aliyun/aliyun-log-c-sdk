#ifndef LIBAOS_HTTP_IO_H
#define LIBAOS_HTTP_IO_H

#include "aos_transport.h"

AOS_CPP_START

aos_http_controller_t *aos_http_controller_create(aos_pool_t *p, int owner);

/* http io error message */
static inline const char *aos_http_controller_get_reason(aos_http_controller_t *ctl)
{
    aos_http_controller_ex_t *ctle = (aos_http_controller_ex_t *)ctl;
    return ctle->reason;
}

CURL *aos_request_get();
void request_release(CURL *request);

int aos_http_io_initialize(const char *user_agent_info, int flag);
void aos_http_io_deinitialize();

int aos_http_send_request(aos_http_controller_t *ctl, aos_http_request_t *req, aos_http_response_t *resp);

void aos_set_default_request_options(aos_http_request_options_t *op);
void aos_set_default_transport_options(aos_http_transport_options_t *op);

aos_http_request_options_t *aos_http_request_options_create(aos_pool_t *p);

aos_http_request_t *aos_http_request_create(aos_pool_t *p);
aos_http_response_t *aos_http_response_create(aos_pool_t *p);

int aos_read_http_body_memory(aos_http_request_t *req, char *buffer, int len);
int aos_write_http_body_memory(aos_http_response_t *resp, const char *buffer, int len);

int aos_read_http_body_file(aos_http_request_t *req, char *buffer, int len);
int aos_write_http_body_file(aos_http_response_t *resp, const char *buffer, int len);

typedef aos_http_transport_t *(*aos_http_transport_create_pt)(aos_pool_t *p);
typedef int (*aos_http_transport_perform_pt)(aos_http_transport_t *t);

extern aos_pool_t *aos_global_pool;
extern apr_file_t *aos_stderr_file;

extern aos_http_request_options_t *aos_default_http_request_options;
extern aos_http_transport_options_t *aos_default_http_transport_options;

extern aos_http_transport_create_pt aos_http_transport_create;
extern aos_http_transport_perform_pt aos_http_transport_perform;

AOS_CPP_END

#endif

