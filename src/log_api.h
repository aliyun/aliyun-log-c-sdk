#ifndef LIBLOG_API_H
#define LIBLOG_API_H

#include "aos_util.h"
#include "aos_string.h"
#include "aos_status.h"
#include "log_define.h"
LOG_CPP_START

aos_status_t *log_post_logs_with_sts_token(aos_pool_t *p, const char *endpoint, const char * accesskeyId, const char *accessKey, const char *stsToken, const char *project, const char *logstore, cJSON *root);
aos_status_t *log_post_logs(aos_pool_t *p, const char *endpoint, const char * accesskeyId, const char *accessKey, const char *project, const char *logstore, cJSON *root);

LOG_CPP_END
#endif
