//
// Created by ZhangCheng on 20/11/2017.
//

#ifndef LOG_C_SDK_LOG_PRODUCER_CONFIG_H
#define LOG_C_SDK_LOG_PRODUCER_CONFIG_H

#include "log_define.h"
#include "log_producer_common.h"
#include "apr_tables.h"
LOG_CPP_START


#define LOG_CONFIG_ENDPOINT "endpoint"
#define LOG_CONFIG_PROJECT "project"
#define LOG_CONFIG_LOGSTORE "logstore"
#define LOG_CONFIG_ACCESS_ID "access_id"
#define LOG_CONFIG_ACCESS_KEY "access_key"
#define LOG_CONFIG_TOKEN "token"
#define LOG_CONFIG_NAME "name"
#define LOG_CONFIG_TAGS "tags"
#define LOG_CONFIG_TOPIC "topic"

#define LOG_CONFIG_LEVEL "level"
#define LOG_CONFIG_LEVEL_DEBUG "DEBUG"
#define LOG_CONFIG_LEVEL_INFO "INFO"
#define LOG_CONFIG_LEVEL_WARN "WARN"
#define LOG_CONFIG_LEVEL_WARNING "WARNING"
#define LOG_CONFIG_LEVEL_ERROR "ERROR"
#define LOG_CONFIG_LEVEL_FATAL "FATAL"

#define LOG_CONFIG_PRIORITY "priority"
#define LOG_CONFIG_PRIORITY_LOWEST "LOWEST"
#define LOG_CONFIG_PRIORITY_LOW "LOW"
#define LOG_CONFIG_PRIORITY_NORMAL "NORMAL"
#define LOG_CONFIG_PRIORITY_HIGH "HIGH"
#define LOG_CONFIG_PRIORITY_HIGHEST "HIGHEST"

#define LOG_CONFIG_PACKAGE_TIMEOUT "package_timeout_ms"
#define LOG_CONFIG_LOG_COUNT_PER_PACKAGE "log_count_per_package"
#define LOG_CONFIG_LOG_BYTES_PER_PACKAGE "log_bytes_per_package"
#define LOG_CONFIG_RETRY_TIME "retry_times"
#define LOG_CONFIG_SEND_THREAD_COUNT "send_thread_count"
#define LOG_CONFIG_MAX_BUFFER_BYTES "max_buffer_bytes"


#define LOG_CONFIG_DEBUG_OPEN "debug_open"
#define LOG_CONFIG_DEBUG_STDOUT "debug_stdout"
#define LOG_CONFIG_DEBUG_LOG_PATH "debug_log_path"
#define LOG_CONFIG_DEBUG_MAX_LOGFILE_COUNT "max_debug_logfile_count"
#define LOG_CONFIG_DEBUG_MAX_LOGFILE_SIZE "max_debug_logfile_size"

#define LOG_CONFIG_SUB_APPENDER "sub_appenders"

// log producer config file sample
//{
//  "endpoint" : "http://{endpoint}",
//  "project" : "{"your_project"}",
//  "logstore" : "{your_logstore}",
//  "access_id" : "{your_access_key_id}",
//  "access_key" : "{your_access_key}",
//  "name" : "test_config",
//  "topic" : "topic_test",
//  "tags" : {
//      "tag_key" : "tag_value",
//      "1": "2"
//  },
//  "level" : "INFO",
//  "package_timeout_ms" : 3000,
//  "log_count_per_package" : 4096,
//  "log_bytes_per_package" : 3000000,
//  "retry_times" : 10,
//  "send_thread_count" : 5,
//  "max_buffer_bytes" : 10000000,
//  "debug_open" : 0,
//  "debug_stdout" : 0,
//  "debug_log_path" : "./log_debug.log",
//  "max_debug_logfile_count" : 5,
//  "max_debug_logfile_size" : 1000000
//}
//

typedef struct _log_producer_config_tag
{
    char * key;
    char * value;
}log_producer_config_tag;

typedef struct _log_producer_config
{
    char * endpoint;
    char * project;
    char * logstore;
    char * accessKeyId;
    char * accessKey;
    char * stsToken;
    char * configName;
    char * topic;
    log_producer_config_tag * tags;
    int32_t tagAllocSize;
    int32_t tagCount;

    int32_t packageTimeoutInMS;
    int32_t logCountPerPackage;
    int32_t logBytesPerPackage;
    int32_t retryTimes;
    int32_t sendThreadCount;
    int32_t maxBufferBytes;
    int32_t debugOpen;
    int32_t debugStdout;
    char * debugLogPath;
    int32_t maxDebugLogFileCount;
    int32_t maxDebugLogFileSize;
    int32_t logLevel;
    log_producer_priority priority;


    apr_pool_t *root;

    apr_table_t * subConfigs;

}log_producer_config;


/**
 * create a empty producer config, should set config params manually
 * @return empty producer config
 */
log_producer_config * create_log_producer_config();

/**
 * create producer config from a config file
 * @param filePullPath config file pull path, must be a valid json
 * @return NULL if fails, otherwise the config
 */
log_producer_config * load_log_producer_config_file(const char * filePullPath);

/**
 * create producer config from a JSON string
 * @param jsonStr a valid json string
 * @return NULL if fail, otherwise the config
 */
log_producer_config * load_log_producer_config_JSON(const char * jsonStr);

/**
 * get sub config
 * @param config
 * @param sub_config
 * @return NULL if fail, otherwise the sub config
 */
log_producer_config * log_producer_config_get_sub(log_producer_config * config, const char * sub_config);

/**
 * set config priority
 * @param config
 * @param priority
 */
void log_producer_config_set_priority(log_producer_config * config, log_producer_priority priority);

/**
 * set producer config endpoint
 * @param config
 * @param endpoint
 */
void log_producer_config_set_endpoint(log_producer_config * config, const char * endpoint);

/**
 * set producer config project
 * @param config
 * @param project
 */
void log_producer_config_set_project(log_producer_config * config, const char * project);

/**
 * set producer config logstore
 * @param config
 * @param logstore
 */
void log_producer_config_set_logstore(log_producer_config * config, const char * logstore);

/**
 * set producer config access id
 * @param config
 * @param access_id
 */
void log_producer_config_set_access_id(log_producer_config * config, const char * access_id);

/**
 * set producer config access key
 * @param config
 * @param access_id
 */
void log_producer_config_set_access_key(log_producer_config * config, const char * access_id);

/**
 * set producer config sts token
 * @param config
 * @param token
 */
void log_producer_config_set_sts_token(log_producer_config * config, const char * token);

/**
 * set producer config name
 * @param config
 * @param config_name
 */
void log_producer_config_set_name(log_producer_config * config, const char * config_name);

/**
 * set producer config topic
 * @param config
 * @param topic
 */
void log_producer_config_set_topic(log_producer_config * config, const char * topic);

/**
 * set producer config param
 * @param config
 * @param open is open
 * @param log_level log level
 * @param stdout_flag is using stdout to print log
 * @param log_path log path
 * @param max_file_count max log file count
 * @param max_file_size max size for a single log file
 */
void log_producer_config_set_debug(log_producer_config * config, int32_t open, int32_t log_level, int32_t stdout_flag, const char * log_path, int32_t max_file_count, int32_t max_file_size);

/**
 * add tag to producer config
 * @param config
 * @param key
 * @param value
 */
void log_producer_config_add_tag(log_producer_config * config, const char * key, const char * value);

/**
 * destroy config, this will free all memory allocated by config's apr pool
 * @param config
 */
void destroy_log_producer_config(log_producer_config * config);

/**
 * print this config
 * @param config
 */
void log_producer_config_print(log_producer_config * config, FILE * pFile);

/**
 * check if given config is valid
 * @param config
 * @return 1 valid, 0 invalid
 */
int log_producer_config_is_valid(log_producer_config * config);



LOG_CPP_END

#endif //LOG_C_SDK_LOG_PRODUCER_CONFIG_H
