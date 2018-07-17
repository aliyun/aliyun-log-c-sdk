//
// Created by ZhangCheng on 20/11/2017.
//

#ifndef LOG_C_SDK_LOG_PRODUCER_MANAGER_H
#define LOG_C_SDK_LOG_PRODUCER_MANAGER_H

#include "log_define.h"
LOG_CPP_START

#include "log_producer_config.h"
#include "log_producer_debug_flusher.h"
#include "log_producer_sender.h"
#include "apr_queue.h"
#include "log_builder.h"
#include "apr_thread_proc.h"
#include "apr_thread_cond.h"
#include "apr_tables.h"


typedef struct _log_producer_manager
{
    apr_pool_t * root;
    apr_pool_t * send_pool;
    log_producer_config * producer_config;
    volatile uint32_t shutdown;
    volatile uint32_t networkRecover;
    volatile apr_uint32_t totalBufferSize;
    log_producer_sender * sender;
    apr_byte_t priority;
    apr_queue_t * loggroup_queue;
    apr_thread_t * flush_thread;
    apr_thread_cond_t * triger_cond;
    apr_thread_mutex_t * triger_lock;
    apr_thread_mutex_t * lock;
    log_group_builder * builder;
    log_producer_debug_flusher * debuger;
    apr_time_t firstLogTime;
    char * source;
    char * pack_prefix;
    volatile uint32_t pack_index;

#ifdef __linux__
    int64_t linux_thread_id;
#endif

    on_log_producer_send_done_function send_done_function;
    apr_table_t * sub_managers;
}log_producer_manager;

extern log_producer_manager * create_log_producer_manager(apr_pool_t * root, log_producer_config * producer_config);
extern void destroy_log_producer_manager(log_producer_manager * manager);

// lock
extern log_producer_result log_producer_manager_add_log_start(log_producer_manager * producer_manager);
extern log_producer_result log_producer_manager_add_log_kv(log_producer_manager * producer_manager, const char * key, const char * value);
extern log_producer_result log_producer_manager_add_log_kv_len(log_producer_manager * producer_manager, const char * key, size_t key_len,  const char * value, size_t value_len);
// unlock
extern log_producer_result log_producer_manager_add_log_end(log_producer_manager * producer_manager);

extern log_producer_result log_producer_manager_send_raw_buffer(log_producer_manager * producer_manager, size_t log_bytes, size_t compressed_bytes, const unsigned char * raw_buffer, size_t log_num);

LOG_CPP_END

#endif //LOG_C_SDK_LOG_PRODUCER_MANAGER_H
