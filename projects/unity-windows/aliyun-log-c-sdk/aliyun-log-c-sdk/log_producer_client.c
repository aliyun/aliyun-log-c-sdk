//
// Created by ZhangCheng on 20/11/2017.
//

#include "log_producer_client.h"
#include "log_producer_manager.h"
#include "inner_log.h"
#include "log_api.h"
#include <stdarg.h>
#include <string.h>
#include "log_persistent_manager.h"

static uint32_t s_init_flag = 0;
static log_producer_result s_last_result = 0;

unsigned int LOG_GET_TIME();

typedef struct _producer_client_private {

    log_producer_manager * producer_manager;
    log_producer_config * producer_config;
    log_persistent_manager * persistent_manager;

}producer_client_private ;

struct _log_producer {
    log_producer_client * root_client;
};

log_producer_result log_producer_env_init()
{
    // if already init, just return s_last_result
    if (s_init_flag == 1)
    {
        return s_last_result;
    }
    s_init_flag = 1;
    if (0 != sls_log_init())
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

log_producer * create_log_producer(log_producer_config * config, on_log_producer_send_done_function send_done_function, void *user_param)
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
    client_private->producer_manager->user_param = user_param;
    client_private->persistent_manager = create_log_persistent_manager(config);
    if (client_private->persistent_manager != NULL)
    {
        client_private->producer_manager->uuid_user_param = client_private->persistent_manager;
        client_private->producer_manager->uuid_send_done_function = on_log_persistent_manager_send_done_uuid;
        int recoverRst = log_persistent_manager_recover(client_private->persistent_manager, client_private->producer_manager);
        if (recoverRst != 0)
        {
            aos_error_log("project %s, logstore %s, recover log persistent manager failed, result %d",
                          config->project,
                          config->logstore,
                          recoverRst);
        }
        else
        {
            aos_info_log("project %s, logstore %s, recover log persistent manager success",
                          config->project,
                          config->logstore);
        }
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
    destroy_log_persistent_manager(client_private->persistent_manager);
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

    log_producer_result rst = log_producer_client_add_log_with_len(client, pairs, keys, key_lens, values, val_lens, 0);
    free(keys);
    free(values);
    free(key_lens);
    free(val_lens);
    return rst;
}

log_producer_result log_producer_client_add_log_with_len(log_producer_client * client, int32_t pair_count, char ** keys, size_t * key_lens, char ** values, size_t * val_lens, int flush)
{
    if (client == NULL || !client->valid_flag)
    {
        return LOG_PRODUCER_INVALID;
    }

    log_producer_manager * manager = ((producer_client_private *)client->private_data)->producer_manager;
    log_persistent_manager * persistent_manager = ((producer_client_private *)client->private_data)->persistent_manager;
    if (persistent_manager != NULL && persistent_manager->is_invalid == 0)
    {
        CS_ENTER(persistent_manager->lock);
        add_log_full(persistent_manager->builder, LOG_GET_TIME(), pair_count, keys, key_lens, values, val_lens);
        char * logBuf = persistent_manager->builder->grp->logs.buffer;
        size_t logSize = persistent_manager->builder->grp->logs.now_buffer_len;
        clear_log_tag(&(persistent_manager->builder->grp->logs));
        if (!log_persistent_manager_is_buffer_enough(persistent_manager, logSize) ||
            manager->totalBufferSize > manager->producer_config->maxBufferBytes)
        {
            CS_LEAVE(persistent_manager->lock);
            return LOG_PRODUCER_DROP_ERROR;
        }
        int rst = log_persistent_manager_save_log(persistent_manager, logBuf, logSize);
        if (rst != LOG_PRODUCER_OK)
        {
            CS_LEAVE(persistent_manager->lock);
            return LOG_PRODUCER_DROP_ERROR;
        }
        rst = log_producer_manager_add_log_raw(manager, logBuf, logSize, flush, persistent_manager->checkpoint.now_log_uuid - 1);
        CS_LEAVE(persistent_manager->lock);
        return rst;
    }

    return log_producer_manager_add_log(manager, pair_count, keys, key_lens, values, val_lens, flush, -1);
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

log_producer_result
log_producer_client_add_log_raw(log_producer_client *client, char *logBuf,
                                size_t logSize, int flush)
{
    if (client == NULL || !client->valid_flag)
    {
        return LOG_PRODUCER_INVALID;
    }
    log_producer_manager * manager = ((producer_client_private *)client->private_data)->producer_manager;
    log_persistent_manager * persistent_manager = ((producer_client_private *)client->private_data)->persistent_manager;
    if (persistent_manager != NULL && persistent_manager->is_invalid == 0)
    {
        CS_ENTER(persistent_manager->lock);
        if (!log_persistent_manager_is_buffer_enough(persistent_manager, logSize) ||
                manager->totalBufferSize > manager->producer_config->maxBufferBytes)
        {
            CS_LEAVE(persistent_manager->lock);
            return LOG_PRODUCER_DROP_ERROR;
        }
        int rst = log_persistent_manager_save_log(persistent_manager, logBuf, logSize);
        if (rst != LOG_PRODUCER_OK)
        {
            CS_LEAVE(persistent_manager->lock);
            return LOG_PRODUCER_DROP_ERROR;
        }
        rst = log_producer_manager_add_log_raw(manager, logBuf, logSize, flush, persistent_manager->checkpoint.now_log_uuid - 1);
        CS_LEAVE(persistent_manager->lock);
        return rst;
    }

    int rst = log_producer_manager_add_log_raw(manager, logBuf, logSize, flush, -1);
    return rst;
}

log_producer_result
log_producer_client_add_log_with_array(log_producer_client *client,
                                       uint32_t logTime, size_t logItemCount,
                                       const char *logItemsBuf,
                                       const uint32_t *logItemsSize, int flush)
{
    if (client == NULL || !client->valid_flag)
    {
        return LOG_PRODUCER_INVALID;
    }
    log_producer_manager * manager = ((producer_client_private *)client->private_data)->producer_manager;
    log_persistent_manager * persistent_manager = ((producer_client_private *)client->private_data)->persistent_manager;
    if (persistent_manager != NULL && persistent_manager->is_invalid == 0)
    {
        CS_ENTER(persistent_manager->lock);

        add_log_full_v2(persistent_manager->builder, logTime, logItemCount, logItemsBuf, logItemsSize);
        char * logBuf = persistent_manager->builder->grp->logs.buffer;
        size_t logSize = persistent_manager->builder->grp->logs.now_buffer_len;
        clear_log_tag(&(persistent_manager->builder->grp->logs));
        if (!log_persistent_manager_is_buffer_enough(persistent_manager, logSize) ||
            manager->totalBufferSize > manager->producer_config->maxBufferBytes)
        {
            CS_LEAVE(persistent_manager->lock);
            return LOG_PRODUCER_DROP_ERROR;
        }
        int rst = log_persistent_manager_save_log(persistent_manager, logBuf, logSize);
        if (rst != LOG_PRODUCER_OK)
        {
            CS_LEAVE(persistent_manager->lock);
            return LOG_PRODUCER_DROP_ERROR;
        }
        rst = log_producer_manager_add_log_raw(manager, logBuf, logSize, flush, persistent_manager->checkpoint.now_log_uuid - 1);
        CS_LEAVE(persistent_manager->lock);
        return rst;
    }

    int rst = log_producer_manager_add_log_with_array(manager, logTime, logItemCount, logItemsBuf, logItemsSize, flush, -1);

    return rst;
}

log_producer_result
log_producer_client_add_log_with_len_int32(log_producer_client *client,
                                           int32_t pair_count, char **keys,
                                           int32_t *key_lens, char **values,
                                           int32_t *value_lens, int flush)
{
    if (client == NULL || !client->valid_flag)
    {
        return LOG_PRODUCER_INVALID;
    }

    log_producer_manager * manager = ((producer_client_private *)client->private_data)->producer_manager;
    log_persistent_manager * persistent_manager = ((producer_client_private *)client->private_data)->persistent_manager;
    if (persistent_manager != NULL && persistent_manager->is_invalid == 0)
    {
        CS_ENTER(persistent_manager->lock);
        add_log_full_int32(persistent_manager->builder, LOG_GET_TIME(), pair_count, keys, key_lens, values, value_lens);
        char * logBuf = persistent_manager->builder->grp->logs.buffer;
        size_t logSize = persistent_manager->builder->grp->logs.now_buffer_len;
        clear_log_tag(&(persistent_manager->builder->grp->logs));
        if (!log_persistent_manager_is_buffer_enough(persistent_manager, logSize) ||
            manager->totalBufferSize > manager->producer_config->maxBufferBytes)
        {
            CS_LEAVE(persistent_manager->lock);
            return LOG_PRODUCER_DROP_ERROR;
        }
        int rst = log_persistent_manager_save_log(persistent_manager, logBuf, logSize);
        if (rst != LOG_PRODUCER_OK)
        {
            CS_LEAVE(persistent_manager->lock);
            return LOG_PRODUCER_DROP_ERROR;
        }
        rst = log_producer_manager_add_log_raw(manager, logBuf, logSize, flush, persistent_manager->checkpoint.now_log_uuid - 1);
        CS_LEAVE(persistent_manager->lock);
        return rst;
    }

    return log_producer_manager_add_log_int32(manager, pair_count, keys, key_lens, values, value_lens, flush, -1);
}


log_producer_result
log_producer_client_add_log_with_len_time_int32(log_producer_client *client,
                                                uint32_t time_sec,
                                                int32_t pair_count, char **keys,
                                                int32_t *key_lens, char **values,
                                                int32_t *value_lens, int flush)
{
    if (client == NULL || !client->valid_flag)
    {
        return LOG_PRODUCER_INVALID;
    }

    log_producer_manager * manager = ((producer_client_private *)client->private_data)->producer_manager;
    log_persistent_manager * persistent_manager = ((producer_client_private *)client->private_data)->persistent_manager;
    if (persistent_manager != NULL && persistent_manager->is_invalid == 0)
    {
        CS_ENTER(persistent_manager->lock);
        add_log_full_int32(persistent_manager->builder, time_sec, pair_count, keys, key_lens, values, value_lens);
        char * logBuf = persistent_manager->builder->grp->logs.buffer;
        size_t logSize = persistent_manager->builder->grp->logs.now_buffer_len;
        clear_log_tag(&(persistent_manager->builder->grp->logs));
        if (!log_persistent_manager_is_buffer_enough(persistent_manager, logSize) ||
            manager->totalBufferSize > manager->producer_config->maxBufferBytes)
        {
            CS_LEAVE(persistent_manager->lock);
            return LOG_PRODUCER_DROP_ERROR;
        }
        int rst = log_persistent_manager_save_log(persistent_manager, logBuf, logSize);
        if (rst != LOG_PRODUCER_OK)
        {
            CS_LEAVE(persistent_manager->lock);
            return LOG_PRODUCER_DROP_ERROR;
        }
        rst = log_producer_manager_add_log_raw(manager, logBuf, logSize, flush, persistent_manager->checkpoint.now_log_uuid - 1);
        CS_LEAVE(persistent_manager->lock);
        return rst;
    }

    return log_producer_manager_add_log_int32(manager, pair_count, keys, key_lens, values, value_lens, flush, -1);
}

