//
// Created by ZhangCheng on 20/11/2017.
//

#include "log_producer_client.h"
#include "log_producer_manager.h"
#include "aos_log.h"
#include "aos_http_io.h"
#include "apr_atomic.h"

#define C_PRODUCER_VERSION "c-producer-0.1.2"

static apr_uint32_t s_init_flag = 0;
static log_producer_result s_last_result = 0;

typedef struct _producer_client_private {

    log_producer_manager * producer_manager;
    log_producer_config * producer_config;
}producer_client_private ;

struct _log_producer {
    apr_pool_t * root;
    log_producer_client * root_client;
    apr_table_t * sub_clients;
};


log_producer_result log_producer_env_init()
{
    // if already init, just return s_last_result
    if (s_init_flag == 1)
    {
        return s_last_result;
    }
    s_init_flag = 1;
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



int _log_producer_create_client_sub(void *rec, const char *key,
                                  const char *value)
{
    log_producer_manager * manager = (log_producer_manager *)value;
    log_producer * producer = (log_producer *)rec;

    log_producer_manager * root_manager = ((producer_client_private *)producer->root_client->private_data)->producer_manager;

    log_producer_client * producer_client = (log_producer_client *)apr_palloc(producer->root, sizeof(log_producer_client));
    producer_client_private * client_private = (producer_client_private *)apr_palloc(producer->root, sizeof(producer_client_private));
    producer_client->private_data = client_private;
    producer_client->log_level = manager->producer_config->logLevel;
    client_private->producer_config = manager->producer_config;
    client_private->producer_manager = manager;
    client_private->producer_manager->send_done_function = root_manager->send_done_function;

    aos_debug_log("create sub producer client success, config : %s", key);
    producer_client->valid_flag = 1;

    apr_table_setn(producer->sub_clients, key, (const char *)producer_client);

    return 1;
}

log_producer * create_log_producer(log_producer_config * config, on_log_producer_send_done_function send_done_function)
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
    log_producer * producer = (log_producer *)apr_palloc(tmp_pool, sizeof(log_producer));
    memset(producer, 0, sizeof(log_producer));
    producer->root = tmp_pool;
    log_producer_client * producer_client = (log_producer_client *)apr_palloc(tmp_pool, sizeof(log_producer_client));
    memset(producer_client, 0, sizeof(log_producer_client));
    producer_client_private * client_private = (producer_client_private *)apr_palloc(tmp_pool, sizeof(producer_client_private));
    memset(client_private, 0, sizeof(producer_client_private));
    producer_client->private_data = client_private;
    producer_client->log_level = config->logLevel;
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
    producer->root_client = producer_client;

    if (client_private->producer_manager->sub_managers != NULL)
    {
        producer->sub_clients = apr_table_make(producer->root, 4);
        apr_table_do(_log_producer_create_client_sub, producer, client_private->producer_manager->sub_managers, NULL);
    }

    return producer;
}

log_producer * create_log_producer_by_config_file(const char * configFilePath, on_log_producer_send_done_function send_done_function)
{
    log_producer_config * config = load_log_producer_config_file(configFilePath);
    if (config == NULL)
    {
        aos_error_log("create config from file %s error.", configFilePath);
        return NULL;
    }
    //log_producer_config_print(config, stdout);
    return create_log_producer(config, send_done_function);
}

int _log_producer_set_invalid_sub(void *rec, const char *key,
                                      const char *value)
{
    log_producer_client * client = (log_producer_client *)value;
    client->valid_flag = 0;
    return 1;
}

void destroy_log_producer(log_producer * producer)
{
    if (producer == NULL)
    {
        return;
    }
    log_producer_client * client = producer->root_client;
    client->valid_flag = 0;
    if (producer->sub_clients != NULL)
    {
        apr_table_do(_log_producer_set_invalid_sub, NULL, producer->sub_clients, NULL);
    }
    producer_client_private * client_private = (producer_client_private *)client->private_data;
    destroy_log_producer_manager(client_private->producer_manager);
    destroy_log_producer_config(client_private->producer_config);
    apr_pool_destroy(producer->root);
}

extern log_producer_client * get_log_producer_client(log_producer * producer, const char * config_name)
{
    if (producer == NULL)
    {
        return NULL;
    }
    if (producer->sub_clients == NULL || config_name == NULL)
    {
        return producer->root_client;
    }
    log_producer_client * client = (log_producer_client *)apr_table_get(producer->sub_clients, config_name);
    if (client == NULL)
    {
        return producer->root_client;
    }
    return client;
}

void log_producer_client_network_recover(log_producer_client * client)
{
    if (client == NULL)
    {
        return;
    }
    log_producer_manager * manager = ((producer_client_private *)client->private_data)->producer_manager;
    manager->networkRecover = 1;
}

log_producer_result log_producer_client_add_log(log_producer_client * client, int32_t kv_count, ...)
{
    if (client == NULL || !client->valid_flag)
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

log_producer_result log_producer_client_add_log_with_len(log_producer_client * client, int32_t pair_count, char ** keys, size_t * key_lens, char ** values, size_t * val_lens)
{
    if (client == NULL || !client->valid_flag)
    {
        return LOG_PRODUCER_INVALID;
    }

    log_producer_manager * manager = ((producer_client_private *)client->private_data)->producer_manager;

    log_producer_result rst = log_producer_manager_add_log_start(manager);
    if (!is_log_producer_result_ok(rst))
    {
        aos_debug_log("create buffer fail, drop this log.");
        return rst;
    }

    int32_t i = 0;
    for (; i < pair_count; ++i)
    {
        log_producer_manager_add_log_kv_len(manager, keys[i], key_lens[i], values[i], val_lens[i]);
    }
    log_producer_manager_add_log_end(manager);
    return LOG_PRODUCER_OK;
}

log_producer_result log_producer_client_add_raw_log_buffer(log_producer_client * client, size_t log_bytes, size_t compressed_bytes, const unsigned char * raw_buffer)
{
    if (client == NULL || !client->valid_flag || raw_buffer == NULL)
    {
        return LOG_PRODUCER_INVALID;
    }

    log_producer_manager * manager = ((producer_client_private *)client->private_data)->producer_manager;

    return log_producer_manager_send_raw_buffer(manager, log_bytes, compressed_bytes, raw_buffer);
}

#ifdef __linux__

pthread_t log_producer_client_get_flush_thread(log_producer_client * client)
{
    if (client == NULL || !client->valid_flag)
    {
        return 0;
    }

    pthread_t * pthread_id = NULL;
    log_producer_manager * manager = ((producer_client_private *)client->private_data)->producer_manager;
    apr_os_thread_get(&pthread_id, manager->flush_thread);

    if (pthread_id != NULL)
    {
        return *pthread_id;
    }
    else
    {
        return -1;
    }
}

int64_t log_producer_client_get_flush_thread_id(log_producer_client * client)
{
    if (client == NULL || !client->valid_flag)
    {
        return 0;
    }

    log_producer_manager * manager = ((producer_client_private *)client->private_data)->producer_manager;
    return manager->linux_thread_id;
}

#endif


