//
// Created by ZhangCheng on 20/11/2017.
//

#ifndef LOG_C_SDK_LOG_PRODUCER_SENDER_H
#define LOG_C_SDK_LOG_PRODUCER_SENDER_H


#include "log_define.h"
#include "log_producer_config.h"
#include "log_builder.h"
LOG_CPP_START

#define LOG_PRODUCER_SEND_MAGIC_NUM 0x1B35487A

typedef int32_t log_producer_send_result;

#define LOG_SEND_OK 0
#define LOG_SEND_NETWORK_ERROR 1
#define LOG_SEND_QUOTA_EXCEED 2
#define LOG_SEND_UNAUTHORIZED 3
#define LOG_SEND_SERVER_ERROR 4
#define LOG_SEND_DISCARD_ERROR 5
#define LOG_SEND_TIME_ERROR 6

extern const char* LOGE_SERVER_BUSY;//= "ServerBusy";
extern const char* LOGE_INTERNAL_SERVER_ERROR;//= "InternalServerError";
extern const char* LOGE_UNAUTHORIZED;//= "Unauthorized";
extern const char* LOGE_WRITE_QUOTA_EXCEED;//="WriteQuotaExceed";
extern const char* LOGE_SHARD_WRITE_QUOTA_EXCEED;//= "ShardWriteQuotaExceed";
extern const char* LOGE_TIME_EXPIRED;//= "RequestTimeExpired";


typedef struct _log_producer_send_param
{
    log_producer_config * producer_config;
    void * producer_manager;
    lz4_log_buf * log_buf;
    uint32_t magic_num;
    uint32_t builder_time;
}log_producer_send_param;

extern void * log_producer_send_fun(void * send_param);

extern log_producer_result log_producer_send_data(log_producer_send_param * send_param);

extern log_producer_send_result AosStatusToResult(post_log_result * result);

extern log_producer_send_param * create_log_producer_send_param(log_producer_config * producer_config,
                                                                void * producer_manager,
                                                                lz4_log_buf * log_buf,
                                                                uint32_t builder_time);

extern log_producer_send_param * create_log_producer_destroy_param(log_producer_config * producer_config, void * producer_manager);

LOG_CPP_END

#endif //LOG_C_SDK_LOG_PRODUCER_SENDER_H
