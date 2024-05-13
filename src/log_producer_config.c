//
// Created by ZhangCheng on 20/11/2017.
//

#include "log_producer_config.h"
#include "sds.h"
#include <string.h>
#include <stdlib.h>
#include "inner_log.h"

static void _set_default_producer_config(log_producer_config * pConfig)
{
    pConfig->logBytesPerPackage = 3 * 1024 * 1024;
    pConfig->logCountPerPackage = 2048;
    pConfig->packageTimeoutInMS = 3000;
    pConfig->maxBufferBytes = 64 * 1024 * 1024;

    pConfig->connectTimeoutSec = 10;
    pConfig->sendTimeoutSec = 15;
    pConfig->destroySenderWaitTimeoutSec = 1;
    pConfig->destroyFlusherWaitTimeoutSec = 1;
    pConfig->compressType = LOG_COMPRESS_LZ4;
    pConfig->using_https = 0;
}


static void _copy_config_string(const char * value, sds * src_value)
{
    if (value == NULL || src_value == NULL)
    {
        return;
    }
    size_t strLen = strlen(value);
    if (*src_value == NULL)
    {
        *src_value = sdsnewEmpty(strLen);
    }
    *src_value = sdscpylen(*src_value, value, strLen);
}


log_producer_config * create_log_producer_config()
{
    log_producer_config* pConfig = (log_producer_config*)malloc(sizeof(log_producer_config));
    memset(pConfig, 0, sizeof(log_producer_config));
    _set_default_producer_config(pConfig);
    pConfig->authVersion = AUTH_VERSION_1;
    pConfig->region = NULL;
    return pConfig;
}


void destroy_log_producer_config(log_producer_config * pConfig)
{
    if (pConfig->project != NULL)
    {
        sdsfree(pConfig->project);
    }
    if (pConfig->logstore != NULL)
    {
        sdsfree(pConfig->logstore);
    }
    if (pConfig->endpoint != NULL)
    {
        sdsfree(pConfig->endpoint);
    }
    if (pConfig->accessKey != NULL)
    {
        sdsfree(pConfig->accessKey);
    }
    if (pConfig->accessKeyId != NULL)
    {
        sdsfree(pConfig->accessKeyId);
    }
    if (pConfig->topic != NULL)
    {
        sdsfree(pConfig->topic);
    }
    if (pConfig->source != NULL)
    {
        sdsfree(pConfig->source);
    }
    if (pConfig->netInterface != NULL)
    {
        sdsfree(pConfig->netInterface);
    }
    if (pConfig->remote_address != NULL)
    {
        sdsfree(pConfig->remote_address);
    }
    if (pConfig->securityToken != NULL)
    {
        sdsfree(pConfig->securityToken);
    }
    if (pConfig->securityTokenLock != NULL)
    {
        DeleteCriticalSection(pConfig->securityTokenLock);
    }
    if (pConfig->tagCount > 0 && pConfig->tags != NULL)
    {
        int i = 0;
        for (; i < pConfig->tagCount; ++i)
        {
            sdsfree(pConfig->tags[i].key);
            sdsfree(pConfig->tags[i].value);
        }
        free(pConfig->tags);
    }
    if (pConfig->region != NULL)
    {
        sdsfree(pConfig->region);
    }
    free(pConfig);
}

#ifdef LOG_PRODUCER_DEBUG
void log_producer_config_print(log_producer_config * pConfig, FILE * file)
{
    fprintf(file, "endpoint : %s\n", pConfig->endpoint);
    fprintf(file,"project : %s\n", pConfig->project);
    fprintf(file,"logstore : %s\n", pConfig->logstore);
    fprintf(file,"accessKeyId : %s\n", pConfig->accessKeyId);
    fprintf(file,"accessKey : %s\n", pConfig->accessKey);
    fprintf(file,"configName : %s\n", pConfig->configName);
    fprintf(file,"topic : %s\n", pConfig->topic);
    fprintf(file,"logLevel : %d\n", pConfig->logLevel);

    fprintf(file,"packageTimeoutInMS : %d\n", pConfig->packageTimeoutInMS);
    fprintf(file, "logCountPerPackage : %d\n", pConfig->logCountPerPackage);
    fprintf(file, "logBytesPerPackage : %d\n", pConfig->logBytesPerPackage);
    fprintf(file, "maxBufferBytes : %d\n", pConfig->maxBufferBytes);


    fprintf(file, "tags: \n");
    int32_t i = 0;
    for (i = 0; i < pConfig->tagCount; ++i)
    {
        fprintf(file, "tag key : %s, value : %s \n", pConfig->tags[i].key, pConfig->tags[i].value);
    }

}
#endif

void log_producer_config_set_packet_timeout(log_producer_config * config, int32_t time_out_ms)
{
    if (config == NULL || time_out_ms < 0)
    {
        return;
    }
    config->packageTimeoutInMS = time_out_ms;
}
void log_producer_config_set_packet_log_count(log_producer_config * config, int32_t log_count)
{
    if (config == NULL || log_count < 0)
    {
        return;
    }
    config->logCountPerPackage = log_count;
}
void log_producer_config_set_packet_log_bytes(log_producer_config * config, int32_t log_bytes)
{
    if (config == NULL || log_bytes < 0)
    {
        return;
    }
    config->logBytesPerPackage = log_bytes;
}
void log_producer_config_set_max_buffer_limit(log_producer_config * config, int64_t max_buffer_bytes)
{
    if (config == NULL || max_buffer_bytes < 0)
    {
        return;
    }
    config->maxBufferBytes = max_buffer_bytes;
}

void log_producer_config_set_send_thread_count(log_producer_config * config, int32_t thread_count)
{
    if (config == NULL || thread_count < 0)
    {
        return;
    }
    config->sendThreadCount = thread_count;
}


void log_producer_config_set_net_interface(log_producer_config * config, const char * net_interface)
{
    if (config == NULL || net_interface == NULL)
    {
        return;
    }
    _copy_config_string(net_interface, &config->netInterface);
}

void log_producer_config_set_remote_address(log_producer_config * config, const char * remote_address)
{
    if (config == NULL || remote_address == NULL)
    {
        return;
    }
    _copy_config_string(remote_address, &config->remote_address);
}

void log_producer_config_set_connect_timeout_sec(log_producer_config * config, int32_t connect_timeout_sec)
{
    if (config == NULL || connect_timeout_sec <= 0)
    {
        return;
    }
    config->connectTimeoutSec = connect_timeout_sec;
}


void log_producer_config_set_send_timeout_sec(log_producer_config * config, int32_t send_timeout_sec)
{
    if (config == NULL || send_timeout_sec <= 0)
    {
        return;
    }
    config->sendTimeoutSec = send_timeout_sec;
}

void log_producer_config_set_destroy_flusher_wait_sec(log_producer_config * config, int32_t destroy_flusher_wait_sec)
{
    if (config == NULL || destroy_flusher_wait_sec <= 0)
    {
        return;
    }
    config->destroyFlusherWaitTimeoutSec = destroy_flusher_wait_sec;
}

void log_producer_config_set_destroy_sender_wait_sec(log_producer_config * config, int32_t destroy_sender_wait_sec)
{
    if (config == NULL || destroy_sender_wait_sec <= 0)
    {
        return;
    }
    config->destroySenderWaitTimeoutSec = destroy_sender_wait_sec;
}

void log_producer_config_set_compress_type(log_producer_config * config, int32_t compress_type)
{
    if (config == NULL ||
        compress_type < (int32_t)LOG_COMPRESS_NONE ||
        compress_type > (int32_t)LOG_COMPRESS_ZSTD)
    {
        return;
    }
    config->compressType = (log_compress_type)compress_type;
}

void log_producer_config_set_using_http(log_producer_config * config, int32_t using_https)
{
    if (config == NULL || using_https < 0)
    {
        return;
    }
    config->using_https = using_https;
}

void log_producer_config_set_log_queue_size(log_producer_config * config, int32_t log_queue_size)
{
    if (config == NULL || log_queue_size < 0)
    {
        return;
    }
    config->logQueueSize = log_queue_size;
}

void log_producer_config_add_tag(log_producer_config * pConfig, const char * key, const char * value)
{
    if(key == NULL || value == NULL)
    {
        return;
    }
    ++pConfig->tagCount;
    if (pConfig->tags == NULL || pConfig->tagCount > pConfig->tagAllocSize)
    {
        if(pConfig->tagAllocSize == 0)
        {
            pConfig->tagAllocSize = 4;
        }
        else
        {
            pConfig->tagAllocSize *= 2;
        }
        log_producer_config_tag * tagArray = (log_producer_config_tag *)malloc(sizeof(log_producer_config_tag) * pConfig->tagAllocSize);
        if (pConfig->tags != NULL)
        {
            memcpy(tagArray, pConfig->tags, sizeof(log_producer_config_tag) * (pConfig->tagCount - 1));
            free(pConfig->tags);
        }
        pConfig->tags = tagArray;
    }
    int32_t tagIndex = pConfig->tagCount - 1;
    pConfig->tags[tagIndex].key = sdsnew(key);
    pConfig->tags[tagIndex].value = sdsnew(value);

}


void log_producer_config_set_endpoint(log_producer_config * config, const char * endpoint)
{
    if (strlen(endpoint) < 8) {
        return;
    }
    if (strncmp(endpoint, "http://", 7) == 0)
    {
        endpoint += 7;
    }
    else if (strncmp(endpoint, "https://", 8) == 0)
    {
        config->using_https = 1;
        endpoint += 8;
    }
    _copy_config_string(endpoint, &config->endpoint);
}

void log_producer_config_set_project(log_producer_config * config, const char * project)
{
    _copy_config_string(project, &config->project);
}
void log_producer_config_set_logstore(log_producer_config * config, const char * logstore)
{
    _copy_config_string(logstore, &config->logstore);
}

void log_producer_config_set_access_id(log_producer_config * config, const char * access_id)
{
    _copy_config_string(access_id, &config->accessKeyId);
}

void log_producer_config_set_access_key(log_producer_config * config, const char * access_key)
{
    _copy_config_string(access_key, &config->accessKey);
}


void log_producer_config_reset_security_token(log_producer_config * config, const char * access_id, const char * access_secret, const char * security_token)
{
    if (config->securityTokenLock == NULL)
    {
        config->securityTokenLock = CreateCriticalSection();
    }
    CS_ENTER(config->securityTokenLock);
    _copy_config_string(access_id, &config->accessKeyId);
    _copy_config_string(access_secret, &config->accessKey);
    _copy_config_string(security_token, &config->securityToken);
    CS_LEAVE(config->securityTokenLock);
}

void log_producer_config_set_auth_version(log_producer_config * config, auth_version auth_version)
{
    config->authVersion = auth_version;
}

void log_producer_config_set_region(log_producer_config * config, const char * region)
{
    _copy_config_string(region, &config->region);
}

void log_producer_config_get_security(log_producer_config * config, char ** access_id, char ** access_secret, char ** security_token)
{
    if (config->securityTokenLock == NULL)
    {
        _copy_config_string(config->accessKeyId, access_id);
        _copy_config_string(config->accessKey, access_secret);
    }
    else
    {
        CS_ENTER(config->securityTokenLock);
        _copy_config_string(config->accessKeyId, access_id);
        _copy_config_string(config->accessKey, access_secret);
        _copy_config_string(config->securityToken, security_token);
        CS_LEAVE(config->securityTokenLock);
    }
}

void log_producer_config_set_topic(log_producer_config * config, const char * topic)
{
    _copy_config_string(topic, &config->topic);
}

void log_producer_config_set_source(log_producer_config * config, const char * source)
{
    _copy_config_string(source, &config->source);
}

int log_producer_config_is_valid(log_producer_config * config)
{
    if (config == NULL)
    {
        aos_error_log("invalid producer config");
        return 0;
    }
    if (config->endpoint == NULL || config->project == NULL || config->logstore == NULL)
    {
        aos_error_log("invalid producer config destination params");
        return 0;
    }
    if (config->accessKey == NULL || config->accessKeyId == NULL)
    {
        aos_error_log("invalid producer config authority params");
        return 0;
    }
    if (config->packageTimeoutInMS < 0 || config->maxBufferBytes < 0 || config->logCountPerPackage < 0 || config->logBytesPerPackage < 0)
    {
        aos_error_log("invalid producer config log merge and buffer params");
        return 0;
    }
    return 1;
}
