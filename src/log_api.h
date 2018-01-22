#ifndef LIBLOG_API_H
#define LIBLOG_API_H

#include "log_define.h"

#include "log_builder.h"
LOG_CPP_START

log_status_t sls_log_init();
void sls_log_destroy();

post_log_result * post_logs_from_lz4buf(const char *endpoint, const char * accesskeyId, const char *accessKey, const char *stsToken, const char *project, const char *logstore, lz4_log_buf * buffer);

void post_log_result_destroy(post_log_result * result);

LOG_CPP_END
#endif
