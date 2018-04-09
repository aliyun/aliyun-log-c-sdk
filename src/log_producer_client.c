//
// Created by ZhangCheng on 20/11/2017.
//

#include "log_producer_client.h"
#include "log_producer_manager.h"
#include "inner_log.h"
#include "log_api.h"
#include <stdarg.h>
#include <string.h>


static uint32_t s_init_flag = 0;
static log_producer_result s_last_result = 0;

typedef struct _producer_client_private {

    log_producer_manager * producer_manager;
    log_producer_config * producer_config;
}producer_client_private ;

struct _log_producer {
    log_producer_client * root_client;
};

log_producer_result log_producer_env_init(int32_t log_global_flag)
{
    // if already init, just return s_last_result
    if (s_init_flag == 1)
    {
        return s_last_result;
    }
    s_init_flag = 1;
    if (0 != sls_log_init(log_global_flag))
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
    if (s_init_flag == 0)
    {
        return;
    }
    s_init_flag = 0;
    sls_log_destroy();
}

log_producer * create_log_producer(log_producer_config * config, on_log_producer_send_done_function send_done_function)
{
    if (!log_producer_config_is_valid(config))
    {
        return NULL;
    }
    log_producer * producer = (log_producer *)malloc(sizeof(log_producer));
    log_producer_client * producer_client = (log_producer_client *)malloc(sizeof(log_producer_client));
    producer_client_private * client_private = (producer_client_private *)malloc(sizeof(producer_client_private));
    producer_client->private_data = client_private;
    client_private->producer_config = config;
    client_private->producer_manager = create_log_producer_manager(config);
    client_private->producer_manager->send_done_function = send_done_function;

    if(client_private->producer_manager == NULL)
    {
        // free
        free(producer_client);
        free(client_private);
        free(producer);
        return NULL;
    }
    aos_debug_log("create producer client success, config : %s", config->logstore);
    producer_client->valid_flag = 1;
    producer->root_client = producer_client;
    return producer;
}


void destroy_log_producer(log_producer * producer)
{
    if (producer == NULL)
    {
        return;
    }
    log_producer_client * client = producer->root_client;
    client->valid_flag = 0;
    producer_client_private * client_private = (producer_client_private *)client->private_data;
    destroy_log_producer_manager(client_private->producer_manager);
    destroy_log_producer_config(client_private->producer_config);
    free(client_private);
    free(client);
    free(producer);
}

extern log_producer_client * get_log_producer_client(log_producer * producer, const char * config_name)
{
    if (producer == NULL)
    {
        return NULL;
    }
    return producer->root_client;
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

    int32_t pairs = kv_count / 2;

    char ** keys = (char **)malloc(pairs * sizeof(char *));
    char ** values = (char **)malloc(pairs * sizeof(char *));
    size_t * key_lens = (size_t *)malloc(pairs * sizeof(size_t));
    size_t * val_lens = (size_t *)malloc(pairs * sizeof(size_t));

    int32_t i = 0;
    for (; i < pairs; ++i)
    {
        const char * key = va_arg(argp, const char *);
        const char * value = va_arg(argp, const char *);
        keys[i] = (char *)key;
        values[i] = (char *)value;
        key_lens[i] = strlen(key);
        val_lens[i] = strlen(value);
    }

    log_producer_manager * manager = ((producer_client_private *)client->private_data)->producer_manager;

    log_producer_result rst = log_producer_manager_add_log(manager, pairs, keys, key_lens, values, val_lens);
    free(keys);
    free(values);
    free(key_lens);
    free(val_lens);
    return rst;
}

log_producer_result log_producer_client_add_log_with_len(log_producer_client * client, int32_t pair_count, char ** keys, size_t * key_lens, char ** values, size_t * val_lens)
{
    if (client == NULL || !client->valid_flag)
    {
        return LOG_PRODUCER_INVALID;
    }

    log_producer_manager * manager = ((producer_client_private *)client->private_data)->producer_manager;

    return log_producer_manager_add_log(manager, pair_count, keys, key_lens, values, val_lens);
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
