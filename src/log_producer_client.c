//
// Created by ZhangCheng on 20/11/2017.
//

#include "log_producer_client.h"
#include "log_producer_manager.h"
#include "aos_log.h"
#include "aos_http_io.h"
#include "apr_atomic.h"

#define C_PRODUCER_VERSION "c-producer-0.1.0"

static apr_uint32_t s_init_flag = 0;
static log_producer_result s_last_result = 0;

typedef struct _producer_client_private {
    apr_pool_t * root;
    log_producer_manager * producer_manager;
    log_producer_config * producer_config;
}producer_client_private ;


log_producer_result log_producer_env_init()
{
    // if already init, just return s_last_result
    if (apr_atomic_xchg32(&s_init_flag, 1))
    {
        return s_last_result;
    }
    if (AOSE_OK != aos_http_io_initialize(C_PRODUCER_VERSION, 0))
    {
        s_last_result = LOG_PRODUCER_INVALID;
    }
    else
    {
        s_last_result = LOG_PRODUCER_OK;
    }
    return s_last_result;
}

void log_producer_env_destroy()
{
    // if already init, just return s_last_result
    if (!apr_atomic_xchg32(&s_init_flag, 0))
    {
        return;
    }
    aos_http_io_deinitialize();
}


log_producer_client * create_log_producer_client(log_producer_config * config, on_log_producer_send_done_function send_done_function)
{
    if (!log_producer_config_is_valid(config))
    {
        return NULL;
    }
    apr_pool_t * tmp_pool = NULL;
    apr_status_t status = apr_pool_create(&tmp_pool, NULL);
    if (tmp_pool == NULL)
    {
        aos_error_log("create log producer client error, can not create pool, error code : %d", status);
        return NULL;
    }
    log_producer_client * producer_client = (log_producer_client *)apr_palloc(tmp_pool, sizeof(log_producer_client));
    producer_client_private * client_private = (producer_client_private *)apr_palloc(tmp_pool, sizeof(producer_client_private));
    producer_client->private_data = client_private;
    producer_client->log_level = config->logLevel;
    client_private->root = tmp_pool;
    client_private->producer_config = config;
    client_private->producer_manager = create_log_producer_manager(tmp_pool, config);
    client_private->producer_manager->send_done_function = send_done_function;

    if(client_private->producer_manager == NULL)
    {
        apr_pool_destroy(tmp_pool);
        return NULL;
    }
    aos_debug_log("create producer client success, config : %s", config->configName);
    producer_client->valid_flag = 1;
    return producer_client;
}

log_producer_client * create_log_producer_client_by_config_file(const char * configFilePath, on_log_producer_send_done_function send_done_function)
{
    log_producer_config * config = load_log_producer_config_file(configFilePath);
    if (config == NULL)
    {
        aos_error_log("create config from file %s error.", configFilePath);
        return NULL;
    }
    //log_producer_config_print(config, stdout);
    return create_log_producer_client(config, send_done_function);
}

void destroy_log_producer_client(log_producer_client * client)
{
    client->valid_flag = 0;
    producer_client_private * client_private = (producer_client_private *)client->private_data;
    destroy_log_producer_manager(client_private->producer_manager);
    destroy_log_producer_config(client_private->producer_config);
    apr_pool_destroy(client_private->root);
}

log_producer_result log_producer_client_add_log(log_producer_client * client, int32_t kv_count, ...)
{
    if (!client->valid_flag)
    {
        return LOG_PRODUCER_INVALID;
    }
    va_list argp;
    va_start(argp, kv_count);

    log_producer_manager * manager = ((producer_client_private *)client->private_data)->producer_manager;

    log_producer_result rst = log_producer_manager_add_log_start(manager);

    if (!is_log_producer_result_ok(rst))
    {
        aos_debug_log("create buffer fail, drop this log.");
        return rst;
    }

    int32_t i = 0;
    int32_t tail = kv_count & 0x01;
    kv_count = (kv_count >> 1)<< 1;
    for (; i < kv_count; i += 2)
    {
        const char * key = va_arg(argp, const char *);                 /*    取出当前的参数，类型为char *. */
        const char * value = va_arg(argp, const char *);
        log_producer_manager_add_log_kv(manager, key, value);
    }
    if (tail)
    {
        const char * key = va_arg(argp, const char *);
        log_producer_manager_add_log_kv(manager, key, "");
    }
    va_end(argp);
    log_producer_manager_add_log_end(manager);
    return LOG_PRODUCER_OK;
}
