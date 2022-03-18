#ifndef LIBLOG_API_H
#define LIBLOG_API_H

#include "log_define.h"

#include "log_builder.h"
LOG_CPP_START

#ifdef WIN32
#undef interface
#endif // WIN32

struct _log_post_option
{
  char * interface;   // net interface to send log, NULL as default
  int connect_timeout; // connection timeout seconds, 0 as default
  int operation_timeout; // operation timeout seconds, 0 as default
  int compress_type; // 0 no compress, 1 lz4
  int ntp_time_offset; //time offset between local time and server time
  int using_https; // 0 http, 1 https
};
typedef struct _log_post_option log_post_option;

log_status_t sls_log_init();
void sls_log_destroy();

post_log_result * post_logs_from_lz4buf(const char *endpoint, const char * accesskeyId, const char *accessKey, const char *stsToken, const char *project, const char *logstore, lz4_log_buf * buffer, log_post_option * option);

post_log_result * post_logs_from_lz4buf_webtracking(const char *endpoint, const char *project, const char *logstore, lz4_log_buf *buffer, log_post_option *option);

void post_log_result_destroy(post_log_result * result);

LOG_CPP_END
#endif
