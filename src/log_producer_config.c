//
// Created by ZhangCheng on 20/11/2017.
//

#include "log_producer_config.h"
#include "cJSON.h"
#include "aos_log.h"
#include "apr_atomic.h"

static void _set_default_producer_config(log_producer_config * pConfig)
{
    pConfig->logBytesPerPackage = 3 * 1024 * 1024;
    pConfig->logCountPerPackage = 2048;
    pConfig->packageTimeoutInMS = 3000;
    pConfig->logLevel = LOG_PRODUCER_LEVEL_INFO;
    pConfig->sendThreadCount = 1;
    pConfig->maxBufferBytes = 64 * 1024 * 1024;
    pConfig->priority = LOG_PRODUCER_PRIORITY_NORMAL;
    pConfig->net_interface = NULL;
    pConfig->connectTimeoutSec = 10;
    pConfig->sendTimeoutSec = 15;
    pConfig->destroySenderWaitTimeoutSec = 1;
    pConfig->destroyFlusherWaitTimeoutSec = 1;
}

static void _set_config_string(apr_pool_t * pool, cJSON * obj, const char * key, char ** value, const char * defaultValue)
{
    cJSON * valItem = cJSON_GetObjectItem(obj, key);
    if (valItem != NULL && valItem->type == cJSON_String)
    {
        size_t strLen = strlen(valItem->valuestring);
        *value = (char *)apr_palloc(pool, strLen + 1);
        strncpy(*value, valItem->valuestring, strLen);
        (*value)[strLen] = '\0';
        return;
    }
    if (defaultValue != NULL)
    {
        size_t strLen = strlen(defaultValue);
        *value = (char *)apr_palloc(pool, strLen + 1);
        strncpy(*value, defaultValue, strLen);
        (*value)[strLen] = '\0';
    }
    return;
}

static void _set_config_int(apr_pool_t * pool, cJSON * obj, const char * key, int32_t * value)
{
    cJSON * valItem = cJSON_GetObjectItem(obj, key);
    if (valItem != NULL && valItem->type == cJSON_Number)
    {
        *value = valItem->valueint;
    }
    return;
}

static void _set_config_tag(log_producer_config * pConfig, cJSON * tagObj)
{
    if (tagObj == NULL)
    {
        return;
    }
    cJSON * c =tagObj->child;
    while (c != NULL)
    {
        if (c->type == cJSON_String)
        {
            // add key value
            log_producer_config_add_tag(pConfig, c->string, c->valuestring);
        }
        c = c->next;
    }
}


static void _copy_config_string(apr_pool_t * pool, const char * value, char ** src_value)
{
    if (value == NULL || src_value == NULL)
    {
        return;
    }
    size_t strLen = strlen(value);
    *src_value = (char *)apr_palloc(pool, strLen + 1);
    strncpy(*src_value, value, strLen);
    (*src_value)[strLen] = '\0';
}

log_producer_config * _create_log_producer_config(apr_pool_t * root)
{
    apr_pool_t *tmp;
    apr_pool_create(&tmp, root);
    log_producer_config* pConfig = (log_producer_config*)apr_palloc(tmp,sizeof(log_producer_config));
    memset(pConfig, 0, sizeof(log_producer_config));
    pConfig->root = tmp;
    apr_status_t status = apr_thread_mutex_create(&(pConfig->tokenLock), APR_THREAD_MUTEX_DEFAULT, pConfig->root);
    assert(status == APR_SUCCESS);
    if (status != APR_SUCCESS)
    {
        aos_fatal_log("create producer config fail on create token lock, error code : %d", status);
    }
    _set_default_producer_config(pConfig);
    return pConfig;
}

log_producer_config * create_log_producer_config()
{
    return _create_log_producer_config(NULL);
}

int log_producer_config_free_sub(void *rec, const char *key,
                                 const char *value)
{
    log_producer_config * producerConfig = (log_producer_config*)value;
    if (producerConfig->stsToken != NULL)
    {
        free(producerConfig->stsToken);
    }
    if (producerConfig->accessKey != NULL)
    {
        free(producerConfig->accessKey);
    }
    if (producerConfig->accessKeyId != NULL)
    {
        free(producerConfig->accessKeyId);
    }
    return 1;
}

void destroy_log_producer_config(log_producer_config * pConfig)
{
    if (pConfig->stsToken != NULL)
    {
        free(pConfig->stsToken);
    }
    if (pConfig->accessKey != NULL)
    {
        free(pConfig->accessKey);
    }
    if (pConfig->accessKeyId != NULL)
    {
        free(pConfig->accessKeyId);
    }
    if (pConfig->subConfigs != NULL)
    {
        apr_table_do(log_producer_config_free_sub, NULL, pConfig->subConfigs, NULL);
    }
    apr_pool_destroy(pConfig->root);
}

int log_producer_config_print_sub(void *rec, const char *key,
                                 const char *value)
{
    FILE * file = (FILE*) rec;
    fprintf(file, "####sub config : %s", key);
    log_producer_config * producerConfig = (log_producer_config*)value;
    log_producer_config_print(producerConfig, file);
    return 1;
}

void log_producer_config_print(log_producer_config * pConfig, FILE * file)
{
    fprintf(file, "endpoint : %s\n", pConfig->endpoint);
    fprintf(file,"project : %s\n", pConfig->project);
    fprintf(file,"logstore : %s\n", pConfig->logstore);
    fprintf(file,"accessKeyId : %s\n", pConfig->accessKeyId);
    fprintf(file,"accessKey : %s\n", pConfig->accessKey);
    fprintf(file,"stsToken : %s\n", pConfig->stsToken == NULL ? "" : pConfig->stsToken);
    fprintf(file,"configName : %s\n", pConfig->configName);
    fprintf(file,"topic : %s\n", pConfig->topic);
    fprintf(file,"logLevel : %d\n", pConfig->logLevel);

    fprintf(file,"packageTimeoutInMS : %d\n", pConfig->packageTimeoutInMS);
    fprintf(file, "logCountPerPackage : %d\n", pConfig->logCountPerPackage);
    fprintf(file, "logBytesPerPackage : %d\n", pConfig->logBytesPerPackage);
    fprintf(file, "retryTimes : %d\n", pConfig->retryTimes);
    fprintf(file, "sendThreadCount : %d\n", pConfig->sendThreadCount);
    fprintf(file, "maxBufferBytes : %d\n", pConfig->maxBufferBytes);

    fprintf(file, "debugOpen : %d\n", pConfig->debugOpen);
    fprintf(file, "debugStdout : %d\n", pConfig->debugStdout);
    fprintf(file, "debugLogPath : %s\n", pConfig->debugLogPath);
    fprintf(file, "maxDebugLogFileCount : %d\n", pConfig->maxDebugLogFileCount);
    fprintf(file, "maxDebugLogFileSize : %d\n", pConfig->maxDebugLogFileSize);

    fprintf(file, "tags: \n");
    int32_t i = 0;
    for (i = 0; i < pConfig->tagCount; ++i)
    {
        fprintf(file, "tag key : %s, value : %s \n", pConfig->tags[i].key, pConfig->tags[i].value);
    }

    if (pConfig->subConfigs != NULL)
    {
        apr_table_do(log_producer_config_print_sub, file, pConfig->subConfigs, NULL);
    }
}


log_producer_config * load_log_producer_config_file(const char * filePullPath)
{
    FILE * pFile = fopen(filePullPath, "r");
    if (pFile == NULL)
    {
        printf("open log producer config file error, %s.\n", strerror(errno));
        return NULL;
    }
    fseek(pFile, 0, SEEK_END);
    int32_t fileSize = ftell(pFile);
    if (fileSize <= 0 || fileSize >= 1024*1024)
    {
        printf("load log producer config file error, file size : %d\n", fileSize);
        fclose(pFile);
        return NULL;
    }
    char * readBuf = (char *)malloc(fileSize + 1);
    readBuf[fileSize] = '\0';
    fseek(pFile, 0, SEEK_SET);
    size_t readSize = fread(readBuf, 1, fileSize, pFile);
    fclose(pFile);
    if (readSize == 0 || readSize > fileSize)
    {
        free(readBuf);
        printf("read log producer config file error, %s.\n", strerror(errno));
        return NULL;
    }
    readBuf[readSize] = '\0';
    log_producer_config * producerConfig = load_log_producer_config_JSON(readBuf);
    free(readBuf);
    return producerConfig;
}

log_producer_config * _load_log_producer_config_JSON_Obj(cJSON * pJson, apr_pool_t * root)
{
    log_producer_config * producerConfig = _create_log_producer_config(root);

    _set_config_string(producerConfig->root, pJson, LOG_CONFIG_ENDPOINT, &(producerConfig->endpoint), "default_endpoint");
    _set_config_string(producerConfig->root, pJson, LOG_CONFIG_PROJECT, &(producerConfig->project), "default_project");
    _set_config_string(producerConfig->root, pJson, LOG_CONFIG_LOGSTORE, &(producerConfig->logstore), "default_logstore");
    _set_config_string(producerConfig->root, pJson, LOG_CONFIG_NAME, &(producerConfig->configName), "default_name");
    _set_config_string(producerConfig->root, pJson, LOG_CONFIG_TOPIC, &(producerConfig->topic), "default_topic");

    cJSON * accessIdItem = cJSON_GetObjectItem(pJson, LOG_CONFIG_ACCESS_ID);
    if (accessIdItem != NULL && accessIdItem->type == cJSON_String)
    {
        log_producer_config_set_access_id(producerConfig, accessIdItem->valuestring);
    }
    else
    {
        log_producer_config_set_access_id(producerConfig, "default_id");
    }

    cJSON * accessKeyItem = cJSON_GetObjectItem(pJson, LOG_CONFIG_ACCESS_KEY);
    if (accessKeyItem != NULL && accessKeyItem->type == cJSON_String)
    {
        log_producer_config_set_access_key(producerConfig, accessKeyItem->valuestring);
    }
    else
    {
        log_producer_config_set_access_key(producerConfig, "default_key");
    }

    cJSON * stsTokenItem = cJSON_GetObjectItem(pJson, LOG_CONFIG_TOKEN);
    if (stsTokenItem != NULL && stsTokenItem->type == cJSON_String)
    {
        log_producer_config_set_sts_token(producerConfig, stsTokenItem->valuestring);
    }

    _set_config_int(producerConfig->root, pJson, LOG_CONFIG_PACKAGE_TIMEOUT, &(producerConfig->packageTimeoutInMS));
    _set_config_int(producerConfig->root, pJson, LOG_CONFIG_LOG_COUNT_PER_PACKAGE, &(producerConfig->logCountPerPackage));
    _set_config_int(producerConfig->root, pJson, LOG_CONFIG_LOG_BYTES_PER_PACKAGE, &(producerConfig->logBytesPerPackage));
    _set_config_int(producerConfig->root, pJson, LOG_CONFIG_RETRY_TIME, &(producerConfig->retryTimes));
    _set_config_int(producerConfig->root, pJson, LOG_CONFIG_SEND_THREAD_COUNT, &(producerConfig->sendThreadCount));
    _set_config_int(producerConfig->root, pJson, LOG_CONFIG_MAX_BUFFER_BYTES, &(producerConfig->maxBufferBytes));

    _set_config_string(producerConfig->root, pJson, LOG_CONFIG_DEBUG_LOG_PATH, &(producerConfig->debugLogPath), NULL);

    _set_config_int(producerConfig->root, pJson, LOG_CONFIG_DEBUG_OPEN, &(producerConfig->debugOpen));
    _set_config_int(producerConfig->root, pJson, LOG_CONFIG_DEBUG_STDOUT, &(producerConfig->debugStdout));
    _set_config_int(producerConfig->root, pJson, LOG_CONFIG_DEBUG_MAX_LOGFILE_COUNT, &(producerConfig->maxDebugLogFileCount));
    _set_config_int(producerConfig->root, pJson, LOG_CONFIG_DEBUG_MAX_LOGFILE_SIZE, &(producerConfig->maxDebugLogFileSize));


    _set_config_string(producerConfig->root, pJson, LOG_CONFIG_NET_INTERFACE, &(producerConfig->net_interface), NULL);
    _set_config_int(producerConfig->root, pJson, LOG_CONFIG_CONNECT_TIMEOUT, &(producerConfig->connectTimeoutSec));
    _set_config_int(producerConfig->root, pJson, LOG_CONFIG_SEND_TIMEOUT, &(producerConfig->sendTimeoutSec));
    _set_config_int(producerConfig->root, pJson, LOG_CONFIG_DESTROY_FLUSHER_WAIT, &(producerConfig->destroyFlusherWaitTimeoutSec));
    _set_config_int(producerConfig->root, pJson, LOG_CONFIG_DESTROY_SENDER_WAIT, &(producerConfig->destroySenderWaitTimeoutSec));

    _set_config_tag(producerConfig, cJSON_GetObjectItem(pJson, LOG_CONFIG_TAGS));


    // level
    cJSON * valItem = cJSON_GetObjectItem(pJson, LOG_CONFIG_LEVEL);
    if (valItem != NULL && valItem->type == cJSON_String)
    {
        if (apr_strnatcasecmp(valItem->valuestring, LOG_CONFIG_LEVEL_DEBUG) == 0)
        {
            producerConfig->logLevel = LOG_PRODUCER_LEVEL_DEBUG;
        }
        else if (apr_strnatcasecmp(valItem->valuestring, LOG_CONFIG_LEVEL_INFO) == 0)
        {
            producerConfig->logLevel = LOG_PRODUCER_LEVEL_INFO;
        }
        else if (apr_strnatcasecmp(valItem->valuestring, LOG_CONFIG_LEVEL_WARN) == 0)
        {
            producerConfig->logLevel = LOG_PRODUCER_LEVEL_WARN;
        }
        else if (apr_strnatcasecmp(valItem->valuestring, LOG_CONFIG_LEVEL_WARNING) == 0)
        {
            producerConfig->logLevel = LOG_PRODUCER_LEVEL_WARN;
        }
        else if (apr_strnatcasecmp(valItem->valuestring, LOG_CONFIG_LEVEL_ERROR) == 0)
        {
            producerConfig->logLevel = LOG_PRODUCER_LEVEL_ERROR;
        }
        else if (apr_strnatcasecmp(valItem->valuestring, LOG_CONFIG_LEVEL_FATAL) == 0)
        {
            producerConfig->logLevel = LOG_PRODUCER_LEVEL_FATAL;
        }
    }

    cJSON * priorityItem = cJSON_GetObjectItem(pJson, LOG_CONFIG_PRIORITY);
    if (priorityItem != NULL && priorityItem->type == cJSON_String)
    {
        if (apr_strnatcasecmp(priorityItem->valuestring, LOG_CONFIG_PRIORITY_LOWEST) == 0)
        {
            producerConfig->priority = LOG_PRODUCER_PRIORITY_LOWEST;
        }
        else if (apr_strnatcasecmp(priorityItem->valuestring, LOG_CONFIG_PRIORITY_LOW) == 0)
        {
            producerConfig->priority = LOG_PRODUCER_PRIORITY_LOW;
        }
        else if (apr_strnatcasecmp(priorityItem->valuestring, LOG_CONFIG_PRIORITY_NORMAL) == 0)
        {
            producerConfig->priority = LOG_PRODUCER_PRIORITY_NORMAL;
        }
        else if (apr_strnatcasecmp(priorityItem->valuestring, LOG_CONFIG_PRIORITY_HIGH) == 0)
        {
            producerConfig->priority = LOG_PRODUCER_PRIORITY_HIGH;
        }
        else if (apr_strnatcasecmp(priorityItem->valuestring, LOG_CONFIG_PRIORITY_HIGHEST) == 0)
        {
            producerConfig->priority = LOG_PRODUCER_PRIORITY_HIGHEST;
        }
    }

    producerConfig->subConfigSize = 0;
    cJSON * subItem = cJSON_GetObjectItem(pJson, LOG_CONFIG_SUB_APPENDER);
    if (subItem != NULL && (subItem->type == cJSON_Object || subItem->type == cJSON_Array))
    {
        producerConfig->subConfigs = apr_table_make(producerConfig->root, 4);
        cJSON *c=subItem->child;
        while (c)
        {
            log_producer_config * subConfig = _load_log_producer_config_JSON_Obj(c, producerConfig->root);
            if (subConfig != NULL)
            {
                apr_table_setn(producerConfig->subConfigs, subConfig->configName, (const char *)subConfig);
                ++producerConfig->subConfigSize;
            }
            c=c->next;
        }
    }

    return producerConfig;
}

log_producer_config * load_log_producer_config_JSON(const char * jsonStr)
{
    cJSON * pJson = cJSON_Parse(jsonStr);
    if (pJson == NULL)
    {
        printf("load log producer config json error, %s.\n", cJSON_GetErrorPtr());
        return NULL;
    }

    log_producer_config * producerConfig = _load_log_producer_config_JSON_Obj(pJson, NULL);
    cJSON_Delete(pJson);
    return producerConfig;
}

log_producer_config * log_producer_config_get_sub(log_producer_config * config, const char * sub_config)
{
    if (config->subConfigs == NULL)
    {
        return NULL;
    }
    return (log_producer_config *)apr_table_get(config->subConfigs, sub_config);
}

void log_producer_config_set_priority(log_producer_config * config, log_producer_priority priority)
{
    if (priority < LOG_PRODUCER_PRIORITY_LOWEST || priority > LOG_PRODUCER_PRIORITY_HIGHEST)
    {
        return;
    }
    config->priority = priority;
}

void log_producer_config_add_tag(log_producer_config * pConfig, const char * key, const char * value)
{
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
        log_producer_config_tag * tagArray = (log_producer_config_tag *)apr_palloc(pConfig->root, sizeof(log_producer_config) * pConfig->tagAllocSize);
        if (pConfig->tags != NULL)
        {
            memcpy(tagArray, pConfig->tags, sizeof(log_producer_config) * (pConfig->tagCount - 1));
        }
        // mem leaks, just for config apr pool
        pConfig->tags = tagArray;
    }
    int32_t tagIndex = pConfig->tagCount - 1;
    size_t keyLen = strlen(key);
    size_t valLen = strlen(value);
    pConfig->tags[tagIndex].key = (char *)apr_palloc(pConfig->root, keyLen + 1);
    strncpy(pConfig->tags[tagIndex].key, key, keyLen);
    pConfig->tags[tagIndex].key[keyLen] = '\0';

    pConfig->tags[tagIndex].value = (char *)apr_palloc(pConfig->root, valLen + 1);
    strncpy(pConfig->tags[tagIndex].value, value, valLen);
    pConfig->tags[tagIndex].value[valLen] = '\0';

}


void log_producer_config_set_debug(log_producer_config * config, int32_t open, int32_t log_level, int32_t stdout_flag, const char * log_path, int32_t max_file_count, int32_t max_file_size)
{
    config->debugOpen = open;
    if (!open)
    {
        return;
    }
    config->logLevel = log_level;
    config->debugStdout = stdout_flag;
    config->maxDebugLogFileCount = max_file_count;
    config->maxDebugLogFileSize = max_file_size;
    _copy_config_string(config->root, log_path, &config->debugLogPath);
}


void log_producer_config_set_endpoint(log_producer_config * config, const char * endpoint)
{
    _copy_config_string(config->root, endpoint, &config->endpoint);
}

void log_producer_config_set_project(log_producer_config * config, const char * project)
{
    _copy_config_string(config->root, project, &config->project);
}
void log_producer_config_set_logstore(log_producer_config * config, const char * logstore)
{
    _copy_config_string(config->root, logstore, &config->logstore);
}

void _update_alloced_string(char ** raw_string, const char * dest_string)
{
    if (dest_string == NULL)
    {
        return;
    }
    if (*raw_string != NULL)
    {
        free(*raw_string);
    }
    *raw_string = strdup(dest_string);
}

void log_producer_config_set_access_id(log_producer_config * config, const char * access_id)
{
    _update_alloced_string(&config->accessKeyId, access_id);
}

void log_producer_config_set_access_key(log_producer_config * config, const char * access_key)
{
    _update_alloced_string(&config->accessKey, access_key);
}

void log_producer_config_set_sts_token(log_producer_config * config, const char * token)
{
    _update_alloced_string(&config->stsToken, token);
}


void log_producer_config_update_token(log_producer_config * config, const char * access_id, const char * access_key, const char * token)
{
    apr_thread_mutex_lock(config->tokenLock);
    log_producer_config_set_access_id(config, access_id);
    log_producer_config_set_access_key(config, access_key);
    log_producer_config_set_sts_token(config, token);
    apr_thread_mutex_unlock(config->tokenLock);
}


void log_producer_config_get_token(log_producer_config * config, char ** access_id, char ** access_key, char ** token)
{
    apr_thread_mutex_lock(config->tokenLock);
    if (config->accessKeyId != NULL)
    {
        *access_id = strdup(config->accessKeyId);
    }
    if (config->accessKey != NULL)
    {
        *access_key = strdup(config->accessKey);
    }
    if (config->stsToken != NULL)
    {
        *token = strdup(config->stsToken);
    }
    apr_thread_mutex_unlock(config->tokenLock);
}


void log_producer_config_set_name(log_producer_config * config, const char * config_name)
{
    _copy_config_string(config->root, config_name, &config->configName);
}

void log_producer_config_set_topic(log_producer_config * config, const char * topic)
{
    _copy_config_string(config->root, topic, &config->topic);
}

int log_producer_config_is_valid(log_producer_config * config)
{
    if (config == NULL || config->root == NULL || config->configName == NULL)
    {
        aos_error_log("invalid producer config");
        return 0;
    }
    if (config->debugOpen)
    {
        if (config->debugLogPath != NULL && config->maxDebugLogFileCount <= 0 && config->maxDebugLogFileSize <= 0)
        {
            aos_error_log("invalid producer config debug params");
            return  0;
        }
    }
    if (config->endpoint == NULL || config->project == NULL || config->logstore == NULL)
    {
        aos_error_log("invalid producer config destination params");
        return 0;
    }
    if (config->stsToken == NULL && (config->accessKey == NULL || config->accessKeyId == NULL))
    {
        aos_error_log("invalid producer config authority params");
        return 0;
    }
    if (config->logLevel <= LOG_PRODUCER_LEVEL_NONE || config->logLevel >= LOG_PRODUCER_LEVEL_MAX)
    {
        aos_error_log("invalid producer config log level params");
        return 0;
    }
    if (config->packageTimeoutInMS < 0 || config->maxBufferBytes < 0 || config->logCountPerPackage < 0 || config->logBytesPerPackage < 0)
    {
        aos_error_log("invalid producer config log merge and buffer params");
        return 0;
    }
    return 1;
}
