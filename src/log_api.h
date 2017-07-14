#ifndef LIBLOG_API_H
#define LIBLOG_API_H

#include "aos_util.h"
#include "aos_string.h"
#include "aos_status.h"
#include "log_define.h"

#include "log_builder.h"
#include "log_http_cont.h"
LOG_CPP_START

struct _log_post_option
{
    char * interface;
};
typedef struct _log_post_option log_post_option;

aos_status_t *log_post_logs_with_sts_token(aos_pool_t *p, const char *endpoint, const char * accesskeyId, const char *accessKey, const char *stsToken, const char *project, const char *logstore, cJSON *root);
aos_status_t *log_post_logs(aos_pool_t *p, const char *endpoint, const char * accesskeyId, const char *accessKey, const char *project, const char *logstore, cJSON *root);
aos_status_t *log_post_logs_from_proto_buf(const char *endpoint, const char * accesskeyId, const char *accessKey, const char *stsToken, const char *project, const char *logstore, log_group_builder* bder);
aos_status_t* log_post_logs_from_http_cont(log_http_cont* cont);

aos_status_t *log_post_logs_with_sts_token_with_option(aos_pool_t *p, const char *endpoint, const char * accesskeyId, const char *accessKey, const char *stsToken, const char *project, const char *logstore, cJSON *root, log_post_option * logPostOption);
aos_status_t *log_post_logs_with_option(aos_pool_t *p, const char *endpoint, const char * accesskeyId, const char *accessKey, const char *project, const char *logstore, cJSON *root, log_post_option * logPostOption);
aos_status_t *log_post_logs_from_proto_buf_with_option(const char *endpoint, const char * accesskeyId, const char *accessKey, const char *stsToken, const char *project, const char *logstore, log_group_builder* bder, log_post_option * logPostOption);
aos_status_t* log_post_logs_from_http_cont_with_option(log_http_cont* cont, log_post_option * logPostOption);

LOG_CPP_END
#endif
