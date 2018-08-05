//
// Created by ZhangCheng on 20/11/2017.
//

#include "log_producer_sender.h"
#include "log_http_cont.h"
#include "log_api.h"
#include "log_producer_manager.h"
#include "apr_atomic.h"
#include "lz4.h"

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
#define SEND_TIME_INVALID_FIX
#define MAX_SENDER_FLUSH_COUNT 100 // 10MS * 100

#define DROP_FAIL_DATA_TIME_SECOND (3600 * 6)

typedef struct _send_error_info
{
    log_producer_send_result last_send_error;
    int32_t last_sleep_ms;
    apr_time_t first_error_time;
}send_error_info;

int32_t log_producer_on_send_done(log_producer_send_param * send_param, aos_status_t * result, send_error_info * error_info);

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
    log_group_builder builder;
    builder.grp = sls_logs__log_group__unpack(NULL, lz4_buf->raw_length, (const uint8_t *)buf);
    free(buf);
    if (builder.grp == NULL)
    {
        aos_fatal_log("sls_logs__log_group__unpack error");
        return;
    }
    int i = 0;
    uint32_t nowTime = time(NULL);
    for(i = 0; i < builder.grp->n_logs; ++i)
    {
        builder.grp->logs[i]->time = nowTime;
    }
    *new_lz4_buf = serialize_to_proto_buf_with_malloc_lz4(&builder);
    sls_logs__log_group__free_unpacked(builder.grp, NULL);
}

#endif

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
        lz4_log_buf * send_buf = send_param->log_buf;
        if (producer_manager->shutdown)
        {
            // @debug
            //continue;
            aos_info_log("send fail but shutdown signal received, force exit");
            if (producer_manager->send_done_function != NULL)
            {
                producer_manager->send_done_function(producer_manager->producer_config->configName, LOG_PRODUCER_SEND_EXIT_BUFFERED, send_param->log_buf->raw_length, send_param->log_buf->length,
                                                     NULL, "producer is being destroyed, producer has no time to send this buffer out", send_param->log_buf->data,
                                                     send_param->log_num);
            }
            break;
        }
#ifdef SEND_TIME_INVALID_FIX
        uint32_t nowTime = time(NULL);
        if (nowTime - send_param->builder_time > 600 || send_param->builder_time > nowTime || error_info.last_send_error == LOG_SEND_TIME_ERROR)
        {
            _rebuild_time(send_param->log_buf, &send_buf);
            send_param->builder_time = nowTime;
        }
#endif

        char * accessId = NULL;
        char * accessKey = NULL;
        char * stsToken = NULL;
        log_producer_config_get_token(config, &accessId, &accessKey, &stsToken);

        log_http_cont* cont =  log_create_http_cont_with_lz4_data(config->endpoint,
                                                                  accessId,
                                                                  accessKey,
                                                                  stsToken,
                                                                  config->project,
                                                                  config->logstore,
                                                                  send_buf,
                                                                  send_pool);
        if (accessId != NULL)
        {
            free(accessId);
        }
        if (accessKey != NULL)
        {
            free(accessKey);
        }
        if (stsToken != NULL)
        {
            free(stsToken);
        }
        log_post_option option;
        memset(&option, 0, sizeof(log_post_option));
        option.interface = config->net_interface;
        option.operation_timeout = config->sendTimeoutSec;
        option.connect_timeout = config->connectTimeoutSec;
        aos_status_t * status =  log_post_logs_from_http_cont_with_option(cont, &option);

        int32_t sleepMs = log_producer_on_send_done(send_param, status, &error_info);

        // tmp buffer, free
        if (send_buf != send_param->log_buf)
        {
            free(send_buf);
        }

        apr_pool_clear(send_pool);
        if (sleepMs <= 0)
        {
            // if send success, return
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
        producer_manager->send_done_function(producer_manager->producer_config->configName, callback_result, send_param->log_buf->raw_length, send_param->log_buf->length,
                                             result->req_id, result->error_msg, send_param->log_buf->data, send_param->log_num);
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
        aos_warn_log("send fail, discard data, project : %s, logstore : %s, config : %s, buffer len : %d, raw len : %d, send queue count: %d, send queue size: %d, total buffer : %d,code : %d, code msg: %s, error msg : %s, req id : %s",
                      send_param->producer_config->project,
                      send_param->producer_config->logstore,
                      send_param->producer_config->configName,
                      (int)send_param->log_buf->length,
                      (int)send_param->log_buf->raw_length,
                      (int)producer_manager->sender->send_queue_count,
                      (int)producer_manager->sender->send_queue_size,
                      (int)producer_manager->totalBufferSize,
                      result->code,
                      result->error_code,
                      result->error_msg,
                      result->req_id);
    }

    return 0;
}

log_producer_result log_producer_send_data(log_producer_send_param * send_param)
{
    log_producer_manager * producer_manager = (log_producer_manager *)send_param->producer_manager;
    apr_uint32_t buf_len = send_param->log_buf->length;
    // if thread pool is null, send data directly
    if (producer_manager->sender->thread_pool == NULL)
    {
        apr_atomic_inc32(&(producer_manager->sender->send_queue_count));
        apr_atomic_add32(&(producer_manager->sender->send_queue_size), buf_len);
        apr_atomic_add32(&(producer_manager->totalBufferSize), buf_len);
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
        apr_atomic_add32(&(producer_manager->sender->send_queue_size), buf_len);
        apr_atomic_add32(&(producer_manager->totalBufferSize), buf_len);

        return LOG_PRODUCER_OK;
    }

    char error_str[128];
    error_str[127] = '\0';

    apr_strerror(result, error_str, 127);

    aos_fatal_log("insert data to send queue fail, error : %s", error_str);

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
    if (strcmp(result->error_code, LOGE_TIME_EXPIRED) == 0)
    {
        return LOG_SEND_TIME_ERROR;
    }
    return LOG_SEND_DISCARD_ERROR;

}

log_producer_sender * create_log_producer_sender(log_producer_config * producer_config, apr_pool_t * root)
{
    log_producer_sender * producer_sender = (log_producer_sender *)apr_palloc(root, sizeof(log_producer_sender));
    memset(producer_sender, 0, sizeof(log_producer_sender));
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

void destroy_log_producer_sender(log_producer_sender * producer_sender, log_producer_config * producer_config)
{
    if (producer_sender->thread_pool == NULL)
    {
        return;
    }

    // add a thread to flush all data out
    apr_thread_pool_thread_max_set(producer_sender->thread_pool, producer_config->sendThreadCount + 1);
    int32_t flush_count = producer_config->destroyFlusherWaitTimeoutSec > 0 ? producer_config->destroyFlusherWaitTimeoutSec * 100 : MAX_SENDER_FLUSH_COUNT;
    int waitCount = 0;
    while (apr_thread_pool_tasks_count(producer_sender->thread_pool) != 0)
    {
        usleep(10 * 1000);
        if (++waitCount == flush_count)
        {
            break;
        }
    }
    if (waitCount == flush_count)
    {
        aos_error_log("can not flush all log in %d second, now task count : %d", flush_count / 100, (int)apr_thread_pool_tasks_count(producer_sender->thread_pool));
    }
    aos_info_log("destroy sender thread pool begin");
    apr_status_t status = apr_thread_pool_destroy(producer_sender->thread_pool);
    if (status != APR_SUCCESS)
    {
        aos_fatal_log("destroy thread pool error, error code : %d", status);
    }
    else
    {
        aos_info_log("destroy sender thread pool success");
    }
}

log_producer_send_param * create_log_producer_send_param(log_producer_config * producer_config,
                                                         void * producer_manager,
                                                         lz4_log_buf * log_buf,
                                                         apr_pool_t * send_pool,
                                                         uint32_t builder_time,
                                                         size_t log_num)
{
    log_producer_send_param * param = (log_producer_send_param *)malloc(sizeof(log_producer_send_param));
    param->producer_config = producer_config;
    param->producer_manager = producer_manager;
    param->log_buf = log_buf;
    param->magic_num = LOG_PRODUCER_SEND_MAGIC_NUM;
    param->send_pool = send_pool;
    param->builder_time = builder_time;
    param->log_num = log_num;
    return param;
}


