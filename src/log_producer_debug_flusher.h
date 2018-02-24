//
// Created by ZhangCheng on 20/11/2017.
//

#ifndef LOG_C_SDK_LOG_PRODUCER_DEBUG_FLUSHER_H
#define LOG_C_SDK_LOG_PRODUCER_DEBUG_FLUSHER_H

#include "log_define.h"
#include "log_producer_config.h"
#include "log_builder.h"
LOG_CPP_START

typedef struct _log_producer_debug_flusher
{
    log_producer_config * config;
    FILE * filePtr;
    int32_t nowFileSize;
    apr_thread_mutex_t * lock;
    apr_pool_t * root;
}log_producer_debug_flusher;

log_producer_debug_flusher * create_log_producer_debug_flusher(log_producer_config * config);
void destroy_log_producer_debug_flusher(log_producer_debug_flusher * producer_debug_flusher);

log_producer_result log_producer_debug_flusher_write(log_producer_debug_flusher * producer_debug_flusher, log_lg * log);
log_producer_result log_producer_debug_flusher_flush(log_producer_debug_flusher * producer_debug_flusher);
int log_producer_print_log(log_lg * log, FILE * pFile);

LOG_CPP_END

#endif //LOG_C_SDK_LOG_PRODUCER_DEBUG_FLUSHER_H
