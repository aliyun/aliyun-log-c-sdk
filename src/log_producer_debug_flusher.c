//
// Created by ZhangCheng on 20/11/2017.
//

#include "log_producer_debug_flusher.h"
#include <stdio.h>

int _log_file_exist(const char * fileName)
{
    FILE * pFile = fopen(fileName, "r");
    if (pFile == NULL)
    {
        return 0;
    }
    fclose(pFile);
    return 1;
}

char * _get_rotate_file_name(const char * srcFile, int index, char * destFile)
{
    sprintf(destFile, "%s.%d", srcFile, index);
    return destFile;
}

log_producer_result _rotate_log_file(log_producer_debug_flusher * producer_debug_flusher)
{
    if (producer_debug_flusher->filePtr != NULL)
    {
        fclose(producer_debug_flusher->filePtr);
        producer_debug_flusher->filePtr = NULL;
    }
    int i = producer_debug_flusher->config->maxDebugLogFileCount - 1;

    char oldFileName[256];
    char newFileName[256];

    remove(_get_rotate_file_name(producer_debug_flusher->config->debugLogPath, producer_debug_flusher->config->maxDebugLogFileCount, oldFileName));

    for (; i > 1; --i)
    {
        rename(_get_rotate_file_name(producer_debug_flusher->config->debugLogPath, i - 1, oldFileName),
               _get_rotate_file_name(producer_debug_flusher->config->debugLogPath, i, newFileName));
    }
    rename(producer_debug_flusher->config->debugLogPath,
           _get_rotate_file_name(producer_debug_flusher->config->debugLogPath, 1, newFileName));

    producer_debug_flusher->filePtr = fopen(producer_debug_flusher->config->debugLogPath, "w");
    return LOG_PRODUCER_OK;
}

log_producer_debug_flusher * create_log_producer_debug_flusher(log_producer_config * config)
{
    if (!config->debugOpen || config->debugLogPath == NULL)
    {
        return NULL;
    }

    log_producer_debug_flusher * producer_debug_flusher = (log_producer_debug_flusher *)apr_pcalloc(config->root, sizeof(log_producer_debug_flusher));
    memset(producer_debug_flusher, 0, sizeof(log_producer_debug_flusher));

    producer_debug_flusher->root = config->root;
    producer_debug_flusher->config = config;
    apr_status_t status = apr_thread_mutex_create(&producer_debug_flusher->lock, APR_THREAD_MUTEX_DEFAULT, producer_debug_flusher->root);
    if (status != APR_SUCCESS)
    {
        return NULL;
    }

    _rotate_log_file(producer_debug_flusher);
    return producer_debug_flusher;
}

void destroy_log_producer_debug_flusher(log_producer_debug_flusher * producer_debug_flusher)
{
    apr_thread_mutex_lock(producer_debug_flusher->lock);
    if (producer_debug_flusher->filePtr != NULL)
    {
        fclose(producer_debug_flusher->filePtr);
    }
    apr_thread_mutex_unlock(producer_debug_flusher->lock);
    apr_thread_mutex_destroy(producer_debug_flusher->lock);
    return;
}

log_producer_result log_producer_debug_flusher_write(log_producer_debug_flusher * producer_debug_flusher, log_lg * log)
{
    apr_thread_mutex_lock(producer_debug_flusher->lock);
    if (producer_debug_flusher->filePtr != NULL)
    {
        producer_debug_flusher->nowFileSize += log_producer_print_log(log, producer_debug_flusher->filePtr);
        if (producer_debug_flusher->nowFileSize >= producer_debug_flusher->config->maxDebugLogFileSize)
        {
            _rotate_log_file(producer_debug_flusher);
            producer_debug_flusher->nowFileSize = 0;
        }
    }
    if (producer_debug_flusher->config->debugStdout)
    {
        log_producer_print_log(log, stdout);
    }
    apr_thread_mutex_unlock(producer_debug_flusher->lock);
    return LOG_PRODUCER_OK;
}

log_producer_result log_producer_debug_flusher_flush(log_producer_debug_flusher * producer_debug_flusher)
{
    apr_thread_mutex_lock(producer_debug_flusher->lock);
    if (producer_debug_flusher->filePtr != NULL)
    {
        fflush(producer_debug_flusher->filePtr);
    }
    apr_thread_mutex_unlock(producer_debug_flusher->lock);
    return LOG_PRODUCER_OK;
}

int log_producer_print_log(log_lg * log, FILE * pFile)
{
    apr_time_t nowTime = (apr_time_t)log->time * (apr_time_t)1000 * (apr_time_t)1000;
    apr_time_exp_t nowTimeExp;
    apr_time_exp_lt(&nowTimeExp, nowTime);
    int nowLen = 23; // with \n
    fprintf(pFile, "[%04d-%02d-%02d %02d:%02d:%02d]\t", nowTimeExp.tm_year + 1900, nowTimeExp.tm_mon, nowTimeExp.tm_mday, nowTimeExp.tm_hour, nowTimeExp.tm_min, nowTimeExp.tm_sec);
    size_t i = 0;
    for (; i < log->n_contents; ++i)
    {
        if (i == log->n_contents - 1)
        {
            fprintf(pFile, "%s:%s\n", log->contents[i]->key, log->contents[i]->value);
        }
        else
        {
            fprintf(pFile, "%s:%s\t", log->contents[i]->key, log->contents[i]->value);
        }
        nowLen += 2 + strlen(log->contents[i]->key) + strlen(log->contents[i]->value);
    }
    return nowLen;
}
