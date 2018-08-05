//
// Created by ZhangCheng on 20/11/2017.
//

#include "log_producer_sender.h"
#include "log_api.h"
#include "log_producer_manager.h"
#include "inner_log.h"
#include "lz4.h"
#include "sds.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const char* LOGE_SERVER_BUSY = "ServerBusy";
const char* LOGE_INTERNAL_SERVER_ERROR = "InternalServerError";
const char* LOGE_UNAUTHORIZED = "Unauthorized";
const char* LOGE_WRITE_QUOTA_EXCEED = "WriteQuotaExceed";
const char* LOGE_SHARD_WRITE_QUOTA_EXCEED = "ShardWriteQuotaExceed";
const char* LOGE_TIME_EXPIRED = "RequestTimeExpired";

#define SEND_SLEEP_INTERVAL_MS 100
#define MAX_NETWORK_ERROR_SLEEP_MS 3600000
#define BASE_NETWORK_ERROR_SLEEP_MS 1000
#define MAX_QUOTA_ERROR_SLEEP_MS 60000
#define BASE_QUOTA_ERROR_SLEEP_MS 3000
#define INVALID_TIME_TRY_INTERVAL 3000

#define DROP_FAIL_DATA_TIME_SECOND (3600 * 6)

#define SEND_TIME_INVALID_FIX

extern volatile uint8_t g_send_thread_destroy;

extern void destroy_log_producer_manager_tail(log_producer_manager * manager);

typedef struct _send_error_info
{
    log_producer_send_result last_send_error;
    int32_t last_sleep_ms;
    int32_t first_error_time;
}send_error_info;

int32_t log_producer_on_send_done(log_producer_send_param * send_param, post_log_result * result, send_error_info * error_info);

#ifdef SEND_TIME_INVALID_FIX

void _rebuild_time(lz4_log_buf * lz4_buf, lz4_log_buf ** new_lz4_buf)
{
    aos_debug_log("rebuild log.");
    char * buf = (char *)malloc(lz4_buf->raw_length);
    if (LZ4_decompress_safe((const char* )lz4_buf->data, buf, lz4_buf->length, lz4_buf->raw_length) <= 0)
    {
        free(buf);
        aos_fatal_log("LZ4_decompress_safe error");
        return;
    }
    uint32_t nowTime = time(NULL);
    fix_log_group_time(buf, lz4_buf->raw_length, nowTime);

    int compress_bound = LZ4_compressBound(lz4_buf->raw_length);
    char *compress_data = (char *)malloc(compress_bound);
    int compressed_size = LZ4_compress_default((char *)buf, compress_data, lz4_buf->raw_length, compress_bound);
    if(compressed_size <= 0)
    {
        aos_fatal_log("LZ4_compress_default error");
        free(buf);
        free(compress_data);
        return;
    }
    *new_lz4_buf = (lz4_log_buf*)malloc(sizeof(lz4_log_buf) + compressed_size);
    (*new_lz4_buf)->length = compressed_size;
    (*new_lz4_buf)->raw_length = lz4_buf->raw_length;
    memcpy((*new_lz4_buf)->data, compress_data, compressed_size);
    free(buf);
    free(compress_data);
    return;
}

#endif

void * log_producer_send_thread(void * param)
{
    log_producer_manager * producer_manager = (log_producer_manager *)param;

    if (producer_manager->sender_data_queue == NULL)
    {
        return NULL;
    }

    while (!producer_manager->shutdown)
    {
        void * send_param = log_queue_pop(producer_manager->sender_data_queue, 30);
        if (send_param != NULL)
        {
            log_producer_send_fun(send_param);
        }
    }

    return NULL;
}

void log_producer_send_thread_global_inner(log_producer_send_param * send_param)
{
    if (send_param != NULL)
    {
        log_producer_manager * producer_manager = (log_producer_manager *)send_param->producer_manager;
        log_producer_send_fun(send_param);
        // @note after log_producer_send_fun, send_param has been destroyed
        if (ATOMICINT_DEC(&producer_manager->ref_count) == 0)
        {
            aos_info_log("producer's ref count is 0, destroy this producer, project : %s, logstore : %s",
                         producer_manager->producer_config->project, producer_manager->producer_config->logstore);
            destroy_log_producer_manager_tail(producer_manager);
        }
    }
}

void * log_producer_send_thread_global(void * param)
{
    log_queue * send_log_queue = (log_queue * )param;

    while (!g_send_thread_destroy)
    {
        log_producer_send_param * send_param = (log_producer_send_param *)log_queue_pop(send_log_queue, 300);
        log_producer_send_thread_global_inner(send_param);
    }

    return NULL;
}

void * log_producer_send_fun(void * param)
{
    log_producer_send_param * send_param = (log_producer_send_param *)param;
    if (send_param->magic_num != LOG_PRODUCER_SEND_MAGIC_NUM)
    {
        aos_fatal_log("invalid send param, magic num not found, num 0x%x", send_param->magic_num);
        return NULL;
    }

    if (send_param->log_buf == NULL)
    {
        aos_info_log("receive producer destroy event, project : %s, logstore : %s", send_param->producer_config->project, send_param->producer_config->logstore);
        free(send_param);
        return NULL;
    }

    log_producer_config * config = send_param->producer_config;

    send_error_info error_info;
    memset(&error_info, 0, sizeof(error_info));

    log_producer_manager * producer_manager = (log_producer_manager *)send_param->producer_manager;

    do
    {
        if (producer_manager->shutdown)
        {
            aos_info_log("send fail but shutdown signal received, force exit");
            if (producer_manager->send_done_function != NULL)
            {
                producer_manager->send_done_function(producer_manager->producer_config->logstore, LOG_PRODUCER_SEND_EXIT_BUFFERED, send_param->log_buf->raw_length, send_param->log_buf->length,
                                                     NULL, "producer is being destroyed, producer has no time to send this buffer out", send_param->log_buf->data);
            }
            break;
        }
        lz4_log_buf * send_buf = send_param->log_buf;
#ifdef SEND_TIME_INVALID_FIX
        uint32_t nowTime = time(NULL);
        if (nowTime - send_param->builder_time > 600 || send_param->builder_time > nowTime || error_info.last_send_error == LOG_SEND_TIME_ERROR)
        {
            _rebuild_time(send_param->log_buf, &send_buf);
            send_param->builder_time = nowTime;
        }
#endif
        log_post_option option;
        memset(&option, 0, sizeof(log_post_option));
        option.connect_timeout = config->connectTimeoutSec;
        option.operation_timeout = config->sendTimeoutSec;
        option.interface = config->netInterface;
        option.compress_type = config->compressType;
        option.remote_address = config->remote_address;
        option.using_https = config->using_https;

        sds accessKeyId = NULL;
        sds accessKey = NULL;
        sds stsToken = NULL;
        log_producer_config_get_security(config, &accessKeyId, &accessKey, &stsToken);

        post_log_result * rst = post_logs_from_lz4buf(config->endpoint, accessKeyId,
                                                      accessKey, stsToken,
                                                      config->project, config->logstore,
                                                      send_buf, &option);
        sdsfree(accessKeyId);
        sdsfree(accessKey);
        sdsfree(stsToken);

        int32_t sleepMs = log_producer_on_send_done(send_param, rst, &error_info);

        post_log_result_destroy(rst);

        // tmp buffer, free
        if (send_buf != send_param->log_buf)
        {
            free(send_buf);
        }

        if (sleepMs <= 0)
        {
            break;
        }
        int i =0;
        for (i = 0; i < sleepMs; i += SEND_SLEEP_INTERVAL_MS)
        {
            usleep(SEND_SLEEP_INTERVAL_MS * 1000);
            if (producer_manager->shutdown || producer_manager->networkRecover)
            {
                break;
            }
        }

        if (producer_manager->networkRecover)
        {
            producer_manager->networkRecover = 0;
        }

    }while(1);

    // at last, free all buffer
    free_lz4_log_buf(send_param->log_buf);
    free(send_param);

    return NULL;

}

int32_t log_producer_on_send_done(log_producer_send_param * send_param, post_log_result * result, send_error_info * error_info)
{
    log_producer_send_result send_result = AosStatusToResult(result);
    log_producer_manager * producer_manager = (log_producer_manager *)send_param->producer_manager;
    if (producer_manager->send_done_function != NULL)
    {
        log_producer_result callback_result = send_result == LOG_SEND_OK ?
                                              LOG_PRODUCER_OK :
                                              (LOG_PRODUCER_SEND_NETWORK_ERROR + send_result - LOG_SEND_NETWORK_ERROR);
        producer_manager->send_done_function(producer_manager->producer_config->logstore,
                                             callback_result,
                                             send_param->log_buf->raw_length,
                                             send_param->log_buf->length,
                                             result->requestID,
                                             result->errorMessage,
                                             send_param->log_buf->data);
    }
    switch (send_result)
    {
        case LOG_SEND_OK:
            break;
        case LOG_SEND_TIME_ERROR:
            // if no this marco, drop data
#ifdef SEND_TIME_INVALID_FIX
            error_info->last_send_error = LOG_SEND_TIME_ERROR;
            error_info->last_sleep_ms = INVALID_TIME_TRY_INTERVAL;
            return error_info->last_sleep_ms;
#else
            break;
#endif
        case LOG_SEND_QUOTA_EXCEED:
            if (error_info->last_send_error != LOG_SEND_QUOTA_EXCEED)
            {
                error_info->last_send_error = LOG_SEND_QUOTA_EXCEED;
                error_info->last_sleep_ms = BASE_QUOTA_ERROR_SLEEP_MS;
                error_info->first_error_time = time(NULL);
            }
            else
            {
                if (error_info->last_sleep_ms < MAX_QUOTA_ERROR_SLEEP_MS)
                {
                    error_info->last_sleep_ms *= 2;
                }

#ifndef SEND_TIME_INVALID_FIX
                // only drop data when SEND_TIME_INVALID_FIX not defined
                if (time(NULL) - error_info->first_error_time > DROP_FAIL_DATA_TIME_SECOND)
                {
                    break;
                }
#endif
            }
            aos_warn_log("send quota error, project : %s, logstore : %s, buffer len : %d, raw len : %d, code : %d, error msg : %s",
                         send_param->producer_config->project,
                         send_param->producer_config->logstore,
                         (int)send_param->log_buf->length,
                         (int)send_param->log_buf->raw_length,
                         result->statusCode,
                         result->errorMessage);
            return error_info->last_sleep_ms;
        case LOG_SEND_SERVER_ERROR :
        case LOG_SEND_NETWORK_ERROR:
            if (error_info->last_send_error != LOG_SEND_NETWORK_ERROR)
            {
                error_info->last_send_error = LOG_SEND_NETWORK_ERROR;
                error_info->last_sleep_ms = BASE_NETWORK_ERROR_SLEEP_MS;
                error_info->first_error_time = time(NULL);
            }
            else
            {
                if (error_info->last_sleep_ms < MAX_NETWORK_ERROR_SLEEP_MS)
                {
                    error_info->last_sleep_ms *= 2;
                }
#ifndef SEND_TIME_INVALID_FIX
                // only drop data when SEND_TIME_INVALID_FIX not defined
                if (time(NULL) - error_info->first_error_time > DROP_FAIL_DATA_TIME_SECOND)
                {
                    break;
                }
#endif
            }
            aos_warn_log("send network error, project : %s, logstore : %s, buffer len : %d, raw len : %d, code : %d, error msg : %s",
                         send_param->producer_config->project,
                         send_param->producer_config->logstore,
                         (int)send_param->log_buf->length,
                         (int)send_param->log_buf->raw_length,
                         result->statusCode,
                         result->errorMessage);
            return error_info->last_sleep_ms;
        default:
            // discard data
            break;

    }

    CS_ENTER(producer_manager->lock);
    producer_manager->totalBufferSize -= send_param->log_buf->length;
    CS_LEAVE(producer_manager->lock);
    if (send_result == LOG_SEND_OK)
    {
        aos_debug_log("send success, project : %s, logstore : %s, buffer len : %d, raw len : %d, total buffer : %d, code : %d, error msg : %s",
                      send_param->producer_config->project,
                      send_param->producer_config->logstore,
                      (int)send_param->log_buf->length,
                      (int)send_param->log_buf->raw_length,
                      (int)producer_manager->totalBufferSize,
                      result->statusCode,
                      result->errorMessage);
    }
    else
    {
        aos_warn_log("send fail, discard data, project : %s, logstore : %s, buffer len : %d, raw len : %d, total buffer : %d, code : %d, error msg : %s",
                      send_param->producer_config->project,
                      send_param->producer_config->logstore,
                      (int)send_param->log_buf->length,
                      (int)send_param->log_buf->raw_length,
                      (int)producer_manager->totalBufferSize,
                      result->statusCode,
                      result->errorMessage);
    }

    return 0;
}

log_producer_result log_producer_send_data(log_producer_send_param * send_param)
{
    log_producer_send_fun(send_param);
    return LOG_PRODUCER_OK;
}

log_producer_send_result AosStatusToResult(post_log_result * result)
{
    if (result->statusCode / 100 == 2)
    {
        return LOG_SEND_OK;
    }
    if (result->statusCode <= 0)
    {
        return LOG_SEND_NETWORK_ERROR;
    }
    if (result->statusCode >= 500 || result->requestID == NULL)
    {
        return LOG_SEND_SERVER_ERROR;
    }
    if (result->statusCode == 403)
    {
        return LOG_SEND_QUOTA_EXCEED;
    }
    if (result->statusCode == 401 || result->statusCode == 404)
    {
        return LOG_SEND_UNAUTHORIZED;
    }
    if (result->errorMessage != NULL && strstr(result->errorMessage, LOGE_TIME_EXPIRED) != NULL)
    {
        return LOG_SEND_TIME_ERROR;
    }
    return LOG_SEND_DISCARD_ERROR;

}

log_producer_send_param * create_log_producer_send_param(log_producer_config * producer_config,
                                                         void * producer_manager,
                                                         lz4_log_buf * log_buf,
                                                         uint32_t builder_time)
{
    log_producer_send_param * param = (log_producer_send_param *)malloc(sizeof(log_producer_send_param));
    param->producer_config = producer_config;
    param->producer_manager = producer_manager;
    param->log_buf = log_buf;
    param->magic_num = LOG_PRODUCER_SEND_MAGIC_NUM;
    param->builder_time = builder_time;
    return param;
}

log_producer_send_param * create_log_producer_destroy_param(log_producer_config * producer_config, void *producer_manager)
{
    log_producer_send_param * param = (log_producer_send_param *)malloc(sizeof(log_producer_send_param));
    param->producer_config = producer_config;
    param->producer_manager = producer_manager;
    param->log_buf = NULL;
    param->magic_num = LOG_PRODUCER_SEND_MAGIC_NUM;
    param->builder_time = 0;
    return param;
}


