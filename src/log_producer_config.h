//
// Created by ZhangCheng on 20/11/2017.
//

#ifndef LOG_C_SDK_LOG_PRODUCER_CONFIG_H
#define LOG_C_SDK_LOG_PRODUCER_CONFIG_H

#include "log_define.h"
#include "log_producer_common.h"
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
LOG_CPP_START



// log producer config file sample
//{
//  "endpoint" : "http://{endpoint}",
//  "project" : "{"your_project"}",
//  "logstore" : "{your_logstore}",
//  "access_id" : "{your_access_key_id}",
//  "access_key" : "{your_access_key}",
//  "topic" : "topic_test",
//  "tags" : {
//      "tag_key" : "tag_value",
//      "1": "2"
//  },
//  "package_timeout_ms" : 3000,
//  "log_count_per_package" : 4096,
//  "log_bytes_per_package" : 3000000,
//  "max_buffer_bytes" : 10000000,
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
    char * topic;
    char * source;
    log_producer_config_tag * tags;
    int32_t tagAllocSize;
    int32_t tagCount;

    int32_t packageTimeoutInMS;
    int32_t logCountPerPackage;
    int32_t logBytesPerPackage;
    int32_t maxBufferBytes;
}log_producer_config;


/**
 * create a empty producer config, should set config params manually
 * @return empty producer config
 */
LOG_EXPORT log_producer_config * create_log_producer_config();


/**
 * set producer config endpoint
 * @param config
 * @param endpoint
 */
LOG_EXPORT void log_producer_config_set_endpoint(log_producer_config * config, const char * endpoint);

/**
 * set producer config project
 * @param config
 * @param project
 */
LOG_EXPORT void log_producer_config_set_project(log_producer_config * config, const char * project);

/**
 * set producer config logstore
 * @param config
 * @param logstore
 */
LOG_EXPORT void log_producer_config_set_logstore(log_producer_config * config, const char * logstore);

/**
 * set producer config access id
 * @param config
 * @param access_id
 */
LOG_EXPORT void log_producer_config_set_access_id(log_producer_config * config, const char * access_id);

/**
 * set producer config access key
 * @param config
 * @param access_id
 */
LOG_EXPORT void log_producer_config_set_access_key(log_producer_config * config, const char * access_id);

/**
 * set producer config topic
 * @param config
 * @param topic
 */
LOG_EXPORT void log_producer_config_set_topic(log_producer_config * config, const char * topic);

/**
 * set producer source, this source will been set to every loggroup's "source" field
 * @param config
 * @param source
 */
LOG_EXPORT void log_producer_config_set_source(log_producer_config * config, const char * source);

/**
 * add tag, this tag will beem set to every loggroup's "tag" field
 * @param config
 * @param key
 * @param value
 */
LOG_EXPORT void log_producer_config_add_tag(log_producer_config * config, const char * key, const char * value);


LOG_EXPORT void log_producer_config_set_packet_timeout(log_producer_config * config, int32_t time_out_ms);
LOG_EXPORT void log_producer_config_set_packet_log_count(log_producer_config * config, int32_t log_count);
LOG_EXPORT void log_producer_config_set_packet_log_bytes(log_producer_config * config, int32_t log_bytes);
LOG_EXPORT void log_producer_config_set_max_buffer_limit(log_producer_config * config, int64_t max_buffer_bytes);



/**
 * destroy config, this will free all memory allocated by config's apr pool
 * @param config
 */
LOG_EXPORT void destroy_log_producer_config(log_producer_config * config);

#ifdef LOG_PRODUCER_DEBUG
/**
 * print this config
 * @param config
 */
void log_producer_config_print(log_producer_config * config, FILE * pFile);

#endif

/**
 * check if given config is valid
 * @param config
 * @return 1 valid, 0 invalid
 */
LOG_EXPORT int log_producer_config_is_valid(log_producer_config * config);



LOG_CPP_END

#endif //LOG_C_SDK_LOG_PRODUCER_CONFIG_H
