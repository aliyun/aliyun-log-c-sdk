//
// Created by ZhangCheng on 20/11/2017.
//

#include "log_producer_sender.h"
#include "log_http_cont.h"
#include "log_api.h"
#include "log_producer_manager.h"
#include "apr_atomic.h"

const char* LOGE_SERVER_BUSY = "ServerBusy";
const char* LOGE_INTERNAL_SERVER_ERROR = "InternalServerError";
const char* LOGE_UNAUTHORIZED = "Unauthorized";
const char* LOGE_WRITE_QUOTA_EXCEED = "WriteQuotaExceed";
const char* LOGE_SHARD_WRITE_QUOTA_EXCEED = "ShardWriteQuotaExceed";

#define SEND_SLEEP_INTERVAL_MS 100
#define MAX_NETWORK_ERROR_SLEEP_MS 3600000
#define BASE_NETWORK_ERROR_SLEEP_MS 1000
#define MAX_QUOTA_ERROR_SLEEP_MS 60000
#define BASE_QUOTA_ERROR_SLEEP_MS 3000

#define DROP_FAIL_DATA_TIME_SECOND (3600 * 6)

typedef struct _send_error_info
{
    log_producer_send_result last_send_error;
    int32_t last_sleep_ms;
    apr_time_t first_error_time;
}send_error_info;

int32_t log_producer_on_send_done(log_producer_send_param * send_param, aos_status_t * result, send_error_info * error_info);

void * log_producer_send_fun(apr_thread_t * thread, void * param)
{
    log_producer_send_param * send_param = (log_producer_send_param *)param;
    if (send_param->magic_num != LOG_PRODUCER_SEND_MAGIC_NUM)
    {
        aos_fatal_log("invalid send param, magic num not found, num 0x%x", send_param->magic_num);
        return NULL;
    }

    log_producer_config * config = send_param->producer_config;

    send_error_info error_info;
    memset(&error_info, 0, sizeof(error_info));

    log_producer_manager * producer_manager = (log_producer_manager *)send_param->producer_manager;

    apr_pool_t * send_pool = send_param->send_pool;
    int32_t create_pool = 0;
    if (send_pool == NULL)
    {
        apr_pool_create(&send_pool, NULL);
        create_pool = 1;
    }
    do
    {
        void * stsTokenPtr = (void *)&config->stsToken;
        log_http_cont* cont =  log_create_http_cont_with_lz4_data(config->endpoint,
                                                                  config->accessKeyId,
                                                                  config->accessKey,
                                                                  (char *)apr_atomic_casptr(stsTokenPtr, NULL, NULL),
                                                                  config->project,
                                                                  config->logstore,
                                                                  send_param->log_buf,
                                                                  send_pool);
        aos_status_t * status =  log_post_logs_from_http_cont_with_option(cont, NULL);

        int32_t sleepMs = log_producer_on_send_done(send_param, status, &error_info);

        apr_pool_clear(send_pool);
        if (sleepMs <= 0)
        {
            // @debug
            //continue;
            break;
        }
        int i =0;
        for (i = 0; i < sleepMs; i += SEND_SLEEP_INTERVAL_MS)
        {
            apr_sleep(SEND_SLEEP_INTERVAL_MS * 1000);
            if (producer_manager->shutdown || producer_manager->networkRecover)
            {
                break;
            }
        }
        if (producer_manager->shutdown)
        {
            // @debug
            //continue;
            aos_info_log("send fail but shutdown signal received, force exit");
            break;
        }
        if (producer_manager->networkRecover)
        {
            producer_manager->networkRecover = 0;
        }

    }while(1);

    if (create_pool)
    {
        apr_pool_destroy(send_pool);
    }

    // at last, free all buffer
    free(send_param->log_buf);
    free(send_param);

    return NULL;

}

int32_t log_producer_on_send_done(log_producer_send_param * send_param, aos_status_t * result, send_error_info * error_info)
{
    log_producer_send_result send_result = AosStatusToResult(result);
    log_producer_manager * producer_manager = (log_producer_manager *)send_param->producer_manager;
    if (producer_manager->send_done_function != NULL)
    {
        log_producer_result callback_result = send_result == LOG_SEND_OK ?
                                              LOG_PRODUCER_OK :
                                              (LOG_PRODUCER_SEND_NETWORK_ERROR + send_result - LOG_SEND_NETWORK_ERROR);
        producer_manager->send_done_function(producer_manager->producer_config->configName, callback_result, send_param->log_buf->raw_length, send_param->log_buf->length, result->req_id, result->error_msg);
    }
    switch (send_result)
    {
        case LOG_SEND_OK:
            break;
        case LOG_SEND_QUOTA_EXCEED:
            if (error_info->last_send_error != LOG_SEND_QUOTA_EXCEED)
            {
                error_info->last_send_error = LOG_SEND_QUOTA_EXCEED;
                error_info->last_sleep_ms = BASE_QUOTA_ERROR_SLEEP_MS;
                error_info->first_error_time = apr_time_now();
            }
            else
            {
                if (error_info->last_sleep_ms < MAX_QUOTA_ERROR_SLEEP_MS)
                {
                    error_info->last_sleep_ms *= 2;
                }
                if (apr_time_now() - error_info->first_error_time > (apr_time_t)DROP_FAIL_DATA_TIME_SECOND * 1000 * 1000)
                {
                    break;
                }
            }
            aos_warn_log("send quota error, project : %s, logstore : %s, config : %s, buffer len : %d, raw len : %d, code : %d, error msg : %s",
                         send_param->producer_config->project,
                         send_param->producer_config->logstore,
                         send_param->producer_config->configName,
                         (int)send_param->log_buf->length,
                         (int)send_param->log_buf->raw_length,
                         result->code,
                         result->error_msg);
            return error_info->last_sleep_ms;
        case LOG_SEND_SERVER_ERROR :
        case LOG_SEND_NETWORK_ERROR:
            if (error_info->last_send_error != LOG_SEND_NETWORK_ERROR)
            {
                error_info->last_send_error = LOG_SEND_NETWORK_ERROR;
                error_info->last_sleep_ms = BASE_NETWORK_ERROR_SLEEP_MS;
                error_info->first_error_time = apr_time_now();
            }
            else
            {
                if (error_info->last_sleep_ms < MAX_NETWORK_ERROR_SLEEP_MS)
                {
                    error_info->last_sleep_ms *= 2;
                }
                if (apr_time_now() - error_info->first_error_time > (apr_time_t)DROP_FAIL_DATA_TIME_SECOND * 1000 * 1000)
                {
                    break;
                }
            }
            aos_warn_log("send network error, project : %s, logstore : %s, config : %s, buffer len : %d, raw len : %d, code : %d, error msg : %s",
                         send_param->producer_config->project,
                         send_param->producer_config->logstore,
                         send_param->producer_config->configName,
                         (int)send_param->log_buf->length,
                         (int)send_param->log_buf->raw_length,
                         result->code,
                         result->error_msg);
            return error_info->last_sleep_ms;
        default:
            break;

    }

    apr_atomic_dec32(&(producer_manager->sender->send_queue_count));
    apr_atomic_sub32(&(producer_manager->sender->send_queue_size), send_param->log_buf->length);
    apr_atomic_sub32(&(producer_manager->totalBufferSize), send_param->log_buf->length);
    if (send_result == LOG_SEND_OK)
    {
        aos_debug_log("send success, project : %s, logstore : %s, config : %s, buffer len : %d, raw len : %d, send queue count: %d, send queue size: %d, total buffer : %d,code : %d, error msg : %s",
                      send_param->producer_config->project,
                      send_param->producer_config->logstore,
                      send_param->producer_config->configName,
                      (int)send_param->log_buf->length,
                      (int)send_param->log_buf->raw_length,
                      (int)producer_manager->sender->send_queue_count,
                      (int)producer_manager->sender->send_queue_size,
                      (int)producer_manager->totalBufferSize,
                      result->code,
                      result->error_msg);
    }
    else
    {
        aos_warn_log("send fail, discard data, project : %s, logstore : %s, config : %s, buffer len : %d, raw len : %d, send queue count: %d, send queue size: %d, total buffer : %d,code : %d, error msg : %s",
                      send_param->producer_config->project,
                      send_param->producer_config->logstore,
                      send_param->producer_config->configName,
                      (int)send_param->log_buf->length,
                      (int)send_param->log_buf->raw_length,
                      (int)producer_manager->sender->send_queue_count,
                      (int)producer_manager->sender->send_queue_size,
                      (int)producer_manager->totalBufferSize,
                      result->code,
                      result->error_msg);
    }

    return 0;
}

log_producer_result log_producer_send_data(log_producer_send_param * send_param)
{
    log_producer_manager * producer_manager = (log_producer_manager *)send_param->producer_manager;
    // if thread pool is null, send data directly
    if (producer_manager->sender->thread_pool == NULL)
    {
        apr_atomic_inc32(&(producer_manager->sender->send_queue_count));
        apr_atomic_add32(&(producer_manager->sender->send_queue_size), send_param->log_buf->length);
        apr_atomic_add32(&(producer_manager->totalBufferSize), send_param->log_buf->length);
        log_producer_send_fun(NULL, send_param);
        return LOG_PRODUCER_OK;
    }
    apr_status_t result = apr_thread_pool_push(producer_manager->sender->thread_pool,
                                               log_producer_send_fun,
                                               send_param,
                                               producer_manager->priority,
                                               send_param->producer_manager);
    if (result == APR_SUCCESS)
    {
        apr_atomic_inc32(&(producer_manager->sender->send_queue_count));
        apr_atomic_add32(&(producer_manager->sender->send_queue_size), send_param->log_buf->length);
        apr_atomic_add32(&(producer_manager->totalBufferSize), send_param->log_buf->length);

        return LOG_PRODUCER_OK;
    }

    char error_str[128];
    error_str[127] = '\0';

    apr_strerror(result, error_str, 127);

    aos_warn_log("insert data to send queue fail, error : %s", error_str);

    return LOG_PRODUCER_WRITE_ERROR;
}

log_producer_send_result AosStatusToResult(aos_status_t * result)
{
    if (aos_status_is_ok(result))
    {
        return LOG_SEND_OK;
    }
    if (result->code <= 0)
    {
        return LOG_SEND_NETWORK_ERROR;
    }
    if (result->code >= 500 || result->error_code == NULL || result->error_msg == NULL)
    {
        return LOG_SEND_SERVER_ERROR;
    }
    if (strcmp(result->error_code, LOGE_SERVER_BUSY) == 0 || strcmp(result->error_code, LOGE_INTERNAL_SERVER_ERROR) == 0)
    {
        return LOG_SEND_SERVER_ERROR;
    }
    if (strcmp(result->error_code, LOGE_WRITE_QUOTA_EXCEED) == 0 || strcmp(result->error_code, LOGE_SHARD_WRITE_QUOTA_EXCEED) == 0)
    {
        return LOG_SEND_QUOTA_EXCEED;
    }
    if (strcmp(result->error_code, LOGE_UNAUTHORIZED) == 0)
    {
        return LOG_SEND_UNAUTHORIZED;
    }
    return LOG_SEND_DISCARD_ERROR;

}

log_producer_sender * create_log_producer_sender(log_producer_config * producer_config, apr_pool_t * root)
{
    log_producer_sender * producer_sender = (log_producer_sender *)apr_palloc(root, sizeof(log_producer_sender));
    producer_sender->send_queue_size = 0;
    producer_sender->send_queue_count = 0;
    producer_sender->thread_pool = NULL;
    apr_status_t status = apr_thread_pool_create(&(producer_sender->thread_pool), producer_config->sendThreadCount, producer_config->sendThreadCount, root);
    if (status != APR_SUCCESS)
    {
        aos_fatal_log("create thread pool error, config : %s, thread count : %d, error code : %d", producer_config->configName, producer_config->sendThreadCount, status);
        // if create thread pool fail, just return producer_sender, and send data directly
        return producer_sender;
    }
    return producer_sender;
}

void destroy_log_producer_sender(log_producer_sender * producer_sender)
{
    if (producer_sender->thread_pool == NULL)
    {
        return;
    }
    int waitCount = 0;
    while (apr_thread_pool_tasks_count(producer_sender->thread_pool) != 0)
    {
        usleep(10 * 1000);
        if (++waitCount == 100)
        {
            break;
        }
    }
    if (waitCount == 100)
    {
        aos_error_log("can not flush all log in 1 second, now task count : %d", (int)apr_thread_pool_tasks_count(producer_sender->thread_pool));
    }

    apr_status_t status = apr_thread_pool_destroy(producer_sender->thread_pool);
    if (status != APR_SUCCESS)
    {
        aos_fatal_log("destroy thread pool error, error code : %d", status);
    }
}

log_producer_send_param * create_log_producer_send_param(log_producer_config * producer_config,
                                                         void * producer_manager,
                                                         lz4_log_buf * log_buf,
                                                         apr_pool_t * send_pool)
{
    log_producer_send_param * param = (log_producer_send_param *)malloc(sizeof(log_producer_send_param));
    param->producer_config = producer_config;
    param->producer_manager = producer_manager;
    param->log_buf = log_buf;
    param->magic_num = LOG_PRODUCER_SEND_MAGIC_NUM;
    param->send_pool = send_pool;
    return param;
}


