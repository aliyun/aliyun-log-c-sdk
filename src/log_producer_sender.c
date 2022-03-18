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
#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

unsigned int LOG_GET_TIME();

const char* LOGE_SERVER_BUSY = "ServerBusy";
const char* LOGE_INTERNAL_SERVER_ERROR = "InternalServerError";
const char* LOGE_UNAUTHORIZED = "Unauthorized";
const char* LOGE_WRITE_QUOTA_EXCEED = "WriteQuotaExceed";
const char* LOGE_SHARD_WRITE_QUOTA_EXCEED = "ShardWriteQuotaExceed";
const char* LOGE_TIME_EXPIRED = "RequestTimeExpired";

#define SEND_SLEEP_INTERVAL_MS 100
#define MAX_NETWORK_ERROR_SLEEP_MS 3000
#define BASE_NETWORK_ERROR_SLEEP_MS 300
#define MAX_QUOTA_ERROR_SLEEP_MS 10000
#define BASE_QUOTA_ERROR_SLEEP_MS 500
#define INVALID_TIME_TRY_INTERVAL 500

#define DROP_FAIL_DATA_TIME_SECOND 86400

#define SEND_TIME_INVALID_FIX

typedef struct _send_error_info
{
    log_producer_send_result last_send_error;
    int32_t last_sleep_ms;
    int32_t first_error_time;
}send_error_info;

int32_t log_producer_on_send_done(log_producer_send_param * send_param, post_log_result * result, send_error_info * error_info);

#ifdef SEND_TIME_INVALID_FIX

void pb_to_webtracking(lz4_log_buf *lz4_buf, lz4_log_buf **new_lz4_buf)
{
    aos_debug_log("pb_to_webtracking start.");
    char * buf = (char *)malloc(lz4_buf->raw_length);
    if (LZ4_decompress_safe((const char* )lz4_buf->data, buf, lz4_buf->length, lz4_buf->raw_length) <= 0)
    {
        free(buf);
        aos_fatal_log("pb_to_webtracking, LZ4_decompress_safe error");
        return;
    }

    size_t len = serialize_pb_buffer_to_webtracking(buf, lz4_buf->raw_length, &buf);

    int compress_bound = LZ4_compressBound(len);
    char *compress_data = (char *)malloc(compress_bound);
    int compressed_size = LZ4_compress_default((char *)buf, compress_data, len, compress_bound);
    if(compressed_size <= 0)
    {
        aos_fatal_log("pb_to_webtracking, LZ4_compress_default error");
        free(buf);
        free(compress_data);
        return;
    }
    *new_lz4_buf = (lz4_log_buf*)malloc(sizeof(lz4_log_buf) + compressed_size);
    (*new_lz4_buf)->length = compressed_size;
    (*new_lz4_buf)->raw_length = len;
    memcpy((*new_lz4_buf)->data, compress_data, compressed_size);
    free(buf);
    free(compress_data);
    aos_debug_log("pb_to_webtracking end.");
}

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
    uint32_t nowTime = LOG_GET_TIME();
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

#ifdef WIN32
DWORD WINAPI log_producer_send_thread(LPVOID param)
#else
void * log_producer_send_thread(void * param)
#endif
{
    log_producer_manager * producer_manager = (log_producer_manager *)param;

    if (producer_manager->sender_data_queue == NULL)
    {
        return 0;
    }

    while (!producer_manager->shutdown)
    {
        // change from 30ms to 1000s, reduce wake up when app switch to back
        void * send_param = log_queue_pop(producer_manager->sender_data_queue, 1000);
        if (send_param != NULL)
        {
            ATOMICINT_INC(&producer_manager->multi_thread_send_count);
            log_producer_send_fun(send_param);
            ATOMICINT_DEC(&producer_manager->multi_thread_send_count);
        }
    }

    return 0;
}

void * log_producer_send_fun(void * param)
{
    aos_info_log("start send log data.");
    log_producer_send_param * send_param = (log_producer_send_param *)param;
    if (send_param->magic_num != LOG_PRODUCER_SEND_MAGIC_NUM)
    {
        aos_fatal_log("invalid send param, magic num not found, num 0x%x", send_param->magic_num);
        log_producer_manager * producer_manager = (log_producer_manager *)send_param->producer_manager;
        if (producer_manager && producer_manager->send_done_function != NULL)
        {
            producer_manager->send_done_function(producer_manager->producer_config->logstore, LOG_PRODUCER_INVALID, send_param->log_buf->raw_length, send_param->log_buf->length,
            NULL, "invalid send param, magic num not found", send_param->log_buf->data, producer_manager->user_param);
        }
        if (producer_manager && producer_manager->uuid_send_done_function != NULL)
        {
            producer_manager->uuid_send_done_function(producer_manager->producer_config->logstore,
                                                 LOG_PRODUCER_INVALID,
                                                 send_param->log_buf->raw_length,
                                                 send_param->log_buf->length,
                                                 NULL,
                                                 "invalid send param, magic num not found",
                                                 send_param->log_buf->data,
                                                 producer_manager->uuid_user_param,
                                                 send_param->start_uuid,
                                                 send_param->end_uuid);
        }
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
                NULL, "producer is being destroyed, producer has no time to send this buffer out", send_param->log_buf->data, producer_manager->user_param);
            }
            break;
        }
        lz4_log_buf * send_buf = send_param->log_buf;
#ifdef SEND_TIME_INVALID_FIX
        uint32_t nowTime = LOG_GET_TIME();
        if (nowTime - send_param->builder_time > 600 || send_param->builder_time - nowTime > 600 || error_info.last_send_error == LOG_SEND_TIME_ERROR)
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
        option.using_https = config->using_https;
        option.ntp_time_offset = config->ntpTimeOffset;
        post_log_result * rst;
        if (config->webTracking)
        {
            pb_to_webtracking(send_param->log_buf, &send_buf);
            rst = post_logs_from_lz4buf_webtracking(config->endpoint, config->project, config->logstore, send_buf, &option);
        }
        else
        {
            sds accessKeyId = NULL;
            sds accessKey = NULL;
            sds stsToken = NULL;
            log_producer_config_get_security(config, &accessKeyId, &accessKey, &stsToken);
            rst = post_logs_from_lz4buf(config->endpoint, accessKeyId, accessKey, stsToken, config->project, config->logstore, send_buf, &option);
            sdsfree(accessKeyId);
            sdsfree(accessKey);
            sdsfree(stsToken);
        }

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
#ifdef WIN32
            Sleep(SEND_SLEEP_INTERVAL_MS);
#else
            usleep(SEND_SLEEP_INTERVAL_MS * 1000);
#endif
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
        producer_manager->send_done_function(producer_manager->producer_config->logstore, callback_result, send_param->log_buf->raw_length, send_param->log_buf->length, result->requestID, result->errorMessage, send_param->log_buf->data, producer_manager->user_param);
    }
    if (producer_manager->uuid_send_done_function != NULL)
    {
        log_producer_result callback_result = send_result == LOG_SEND_OK ?
                                              LOG_PRODUCER_OK :
                                              (LOG_PRODUCER_SEND_NETWORK_ERROR + send_result - LOG_SEND_NETWORK_ERROR);
        producer_manager->uuid_send_done_function(producer_manager->producer_config->logstore,
                                                  callback_result,
                                                  send_param->log_buf->raw_length,
                                                  send_param->log_buf->length,
                                                  result->requestID,
                                                  result->errorMessage,
                                                  send_param->log_buf->data,
                                                  producer_manager->uuid_user_param,
                                                  send_param->start_uuid,
                                                  send_param->end_uuid);
    }
    if (send_result == LOG_SEND_UNAUTHORIZED)
    {
        // if do not drop unauthorized log, change the code to LOG_PRODUCER_SEND_NETWORK_ERROR
        if (producer_manager->producer_config->dropUnauthorizedLog == 0)
        {
            send_result = LOG_PRODUCER_SEND_NETWORK_ERROR;
        }
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
                if (time(NULL) - error_info->first_error_time > DROP_FAIL_DATA_TIME_SECOND)
                {
                    break;
                }
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
        case LOG_SEND_PARAMETERS_ERROR:
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
                if (time(NULL) - error_info->first_error_time > DROP_FAIL_DATA_TIME_SECOND)
                {
                    break;
                }
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

    // always try once when discard error
    if (LOG_SEND_OK != send_result && error_info->last_send_error == 0)
    {
        error_info->last_send_error = LOG_SEND_DISCARD_ERROR;
        error_info->last_sleep_ms = BASE_NETWORK_ERROR_SLEEP_MS;
        error_info->first_error_time = time(NULL);
        aos_warn_log("send fail, the error is discard data, retry once, project : %s, logstore : %s, buffer len : %d, raw len : %d, total buffer : %d,code : %d, error msg : %s",
                     send_param->producer_config->project,
                     send_param->producer_config->logstore,
                     (int)send_param->log_buf->length,
                     (int)send_param->log_buf->raw_length,
                     (int)producer_manager->totalBufferSize,
                     result->statusCode,
                     result->errorMessage);
        return BASE_NETWORK_ERROR_SLEEP_MS;
    }

    CS_ENTER(producer_manager->lock);
    producer_manager->totalBufferSize -= send_param->log_buf->length;
    CS_LEAVE(producer_manager->lock);
    if (send_result == LOG_SEND_OK)
    {
        aos_debug_log("send success, project : %s, logstore : %s, buffer len : %d, raw len : %d, total buffer : %d,code : %d, error msg : %s",
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
        aos_warn_log("send fail, discard data, project : %s, logstore : %s, buffer len : %d, raw len : %d, total buffer : %d,code : %d, error msg : %s",
                      send_param->producer_config->project,
                      send_param->producer_config->logstore,
                      (int)send_param->log_buf->length,
                      (int)send_param->log_buf->raw_length,
                      (int)producer_manager->totalBufferSize,
                      result->statusCode,
                      result->errorMessage);
        if (producer_manager->send_done_function != NULL)
        {
            producer_manager->send_done_function(producer_manager->producer_config->logstore,
                                                 LOG_PRODUCER_DROP_ERROR,
                                                 send_param->log_buf->raw_length,
                                                 send_param->log_buf->length,
                                                 result->requestID,
                                                 result->errorMessage,
                                                 send_param->log_buf->data,
                                                 producer_manager->user_param);
        }
        if (producer_manager->uuid_send_done_function != NULL)
        {
            producer_manager->uuid_send_done_function(producer_manager->producer_config->logstore,
                                                      LOG_PRODUCER_DROP_ERROR,
                                                      send_param->log_buf->raw_length,
                                                      send_param->log_buf->length,
                                                      result->requestID,
                                                      result->errorMessage,
                                                      send_param->log_buf->data,
                                                      producer_manager->uuid_user_param,
                                                      send_param->start_uuid,
                                                      send_param->end_uuid);
        }
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
    if (result->statusCode == 405)
    {
        return LOG_SEND_PARAMETERS_ERROR;
    }
    if (result->statusCode == 403)
    {
        return LOG_SEND_QUOTA_EXCEED;
    }
    if (result->statusCode == 401 || result->statusCode == 404)
    {
        return LOG_SEND_UNAUTHORIZED;
    }
    if (result->statusCode >= 500 || result->requestID == NULL)
    {
        return LOG_SEND_SERVER_ERROR;
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
                                                         log_group_builder * builder)
{
    log_producer_send_param * param = (log_producer_send_param *)malloc(sizeof(log_producer_send_param));
    param->producer_config = producer_config;
    param->producer_manager = producer_manager;
    param->log_buf = log_buf;
    param->magic_num = LOG_PRODUCER_SEND_MAGIC_NUM;
    if (builder != NULL)
    {
        param->builder_time = builder->builder_time;
        param->start_uuid = builder->start_uuid;
        param->end_uuid = builder->end_uuid;
    }
    else
    {
        param->builder_time = time(NULL);
        param->start_uuid = -1;
        param->end_uuid = -1;
    }
    return param;
}


