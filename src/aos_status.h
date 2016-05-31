#ifndef LIBAOS_STATUS_H
#define LIBAOS_STATUS_H

#include "aos_define.h"
#include "aos_list.h"

AOS_CPP_START

typedef struct aos_status_s aos_status_t;

struct aos_status_s {
    int code; // > 0 http code
    char *error_code; // can't modify
    char *error_msg; // can't modify
    char *req_id;   // can't modify
};

static inline int aos_status_is_ok(aos_status_t *s)
{
    return s->code > 0 && s->code / 100 == 2;
}

static inline int aos_http_is_ok(int st)
{
    return st / 100 == 2;
}

#define aos_status_set(s, c, ec, es)                                    \
    (s)->code = c; (s)->error_code = (char *)ec; (s)->error_msg = (char *)es

aos_status_t *aos_status_create(aos_pool_t *p);

aos_status_t *aos_status_dup(aos_pool_t *p, aos_status_t *src);

aos_status_t *aos_status_parse_from_body(aos_pool_t *p, aos_list_t *bc, int code, aos_status_t *s);

extern const char AOS_OPEN_FILE_ERROR_CODE[];
extern const char AOS_HTTP_IO_ERROR_CODE[];
extern const char AOS_UNKNOWN_ERROR_CODE[];
extern const char AOS_CLIENT_ERROR_CODE[];
extern const char AOS_UTF8_ENCODE_ERROR_CODE[];

AOS_CPP_END

#endif
