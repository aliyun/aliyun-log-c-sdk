//
// Created by ZhangCheng on 20/11/2017.
//

#ifndef LOG_C_SDK_LOG_PRODUCER_SENDER_H
#define LOG_C_SDK_LOG_PRODUCER_SENDER_H


#include "log_define.h"
#include "apr_thread_pool.h"
#include "apr_atomic.h"
#include "log_producer_config.h"
#include "log_builder.h"
#include "aos_status.h"
LOG_CPP_START

#define LOG_PRODUCER_SEND_MAGIC_NUM 0x1B35487A

typedef int32_t log_producer_send_result;

#define LOG_SEND_OK 0
#define LOG_SEND_NETWORK_ERROR 1
#define LOG_SEND_QUOTA_EXCEED 2
#define LOG_SEND_UNAUTHORIZED 3
#define LOG_SEND_SERVER_ERROR 4
#define LOG_SEND_DISCARD_ERROR 5

extern const char* LOGE_SERVER_BUSY;//= "ServerBusy";
extern const char* LOGE_INTERNAL_SERVER_ERROR;//= "InternalServerError";
extern const char* LOGE_UNAUTHORIZED;//= "Unauthorized";
extern const char* LOGE_WRITE_QUOTA_EXCEED;//="WriteQuotaExceed";
extern const char* LOGE_SHARD_WRITE_QUOTA_EXCEED;//= "ShardWriteQuotaExceed";

typedef struct _log_producer_sender
{
    apr_thread_pool_t * thread_pool;
    volatile apr_uint32_t send_queue_count;
    volatile apr_uint32_t send_queue_size;
}log_producer_sender;


typedef struct _log_producer_send_param
{
    log_producer_config * producer_config;
    void * producer_manager;
    lz4_log_buf * log_buf;
    uint32_t magic_num;
    apr_pool_t * send_pool;
}log_producer_send_param;

extern void * APR_THREAD_FUNC log_producer_send_fun(apr_thread_t * thread, void * send_param);

extern log_producer_result log_producer_send_data(log_producer_send_param * send_param);

extern log_producer_send_result AosStatusToResult(aos_status_t * result);

extern log_producer_sender * create_log_producer_sender(log_producer_config * producer_config, apr_pool_t * root);

extern void destroy_log_producer_sender(log_producer_sender * producer_sender);

extern log_producer_send_param * create_log_producer_send_param(log_producer_config * producer_config,
                                                                void * producer_manager,
                                                                lz4_log_buf * log_buf,
                                                                apr_pool_t * send_pool);

LOG_CPP_END

#endif //LOG_C_SDK_LOG_PRODUCER_SENDER_H
