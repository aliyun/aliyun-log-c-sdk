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
#include "log_multi_thread.h"
LOG_CPP_START


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
    char * securityToken;
    char * topic;
    char * source;
    auth_version authVersion;
    char * region;
    CRITICALSECTION securityTokenLock;
    log_producer_config_tag * tags;
    int32_t tagAllocSize;
    int32_t tagCount;

    int32_t sendThreadCount;

    int32_t packageTimeoutInMS;
    int32_t logCountPerPackage;
    int32_t logBytesPerPackage;
    int32_t maxBufferBytes;
    int32_t logQueueSize;

    char * netInterface;
    char * remote_address;
    int32_t connectTimeoutSec;
    int32_t sendTimeoutSec;
    int32_t destroyFlusherWaitTimeoutSec;
    int32_t destroySenderWaitTimeoutSec;

    log_compress_type compressType;
    int32_t using_https; // default http, 0 http, 1 https

}log_producer_config;


/**
 * create a empty producer config, should set config params manually
 * @return empty producer config
 */
LOG_EXPORT log_producer_config * create_log_producer_config();


/**
 * set producer config endpoint
 * @note if endpoint start with "https", then set using_https 1
 * @note strlen(endpoint) must >= 8
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
 * set producer config auth_version
 * @param config
 * @param auth_version
 */
LOG_EXPORT void log_producer_config_set_auth_version(log_producer_config * config, auth_version auth_version);


/**
 * set producer config region
 * @param config
 * @param region
 */
LOG_EXPORT void log_producer_config_set_region(log_producer_config * config, const char * region);


/**
 * reset producer config security token (thread safe)
 * @note if you want to use security token to send logs, you must call this function when create config and reset token before expired.
 *       if token has expired, producer will drop logs after 6 hours
 * @param config
 * @param access_id
 */
LOG_EXPORT void log_producer_config_reset_security_token(log_producer_config * config, const char * access_id, const char * access_secret, const char * security_token);


/**
 * inner api
 */
LOG_EXPORT void log_producer_config_get_security(log_producer_config * config, char ** access_id, char ** access_secret, char ** security_token);

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
 * add tag, this tag will been set to every loggroup's "tag" field
 * @param config
 * @param key
 * @param value
 */
LOG_EXPORT void log_producer_config_add_tag(log_producer_config * config, const char * key, const char * value);

/**
 * set loggroup timeout, if delta time from first log's time >= time_out_ms, current loggroup will been flushed out
 * @param config
 * @param time_out_ms
 */
LOG_EXPORT void log_producer_config_set_packet_timeout(log_producer_config * config, int32_t time_out_ms);

/**
 * set loggroup max log count, if loggoup's logs count  >= log_count, current loggroup will been flushed out
 * @param config
 * @param log_count
 */
LOG_EXPORT void log_producer_config_set_packet_log_count(log_producer_config * config, int32_t log_count);

/**
 * set loggroup max log bytes, if loggoup's log bytes  >= log_bytes, current loggroup will been flushed out
 * @param config
 * @param log_bytes
 */
LOG_EXPORT void log_producer_config_set_packet_log_bytes(log_producer_config * config, int32_t log_bytes);

/**
 * set max buffer size, if total buffer size > max_buffer_bytes, all send will fail immediately
 * @param config
 * @param max_buffer_bytes
 */
LOG_EXPORT void log_producer_config_set_max_buffer_limit(log_producer_config * config, int64_t max_buffer_bytes);

/**
 * set send thread count, default is 0.
 * @note if thread count is 0, flusher thread is in the charge of send logs.
 * @note if thread count > 1, then producer will create $(thread_count) threads to send logs.
 * @param config
 * @param thread_count
 */
LOG_EXPORT void log_producer_config_set_send_thread_count(log_producer_config * config, int32_t thread_count);

/**
 * set interface to send log out
 * @param config
 * @param net_interface
 */
LOG_EXPORT void log_producer_config_set_net_interface(log_producer_config * config, const char * net_interface);

/**
 * set remote_address to send log out
 * @param config
 * @param remote_address
 */
LOG_EXPORT void log_producer_config_set_remote_address(log_producer_config * config, const char * remote_address);

/**
 * set connect timeout seconds
 * @param config
 * @param connect_timeout_sec
 */
LOG_EXPORT void log_producer_config_set_connect_timeout_sec(log_producer_config * config, int32_t connect_timeout_sec);

/**
 * set send timeout seconds
 * @param config
 * @param send_timeout_sec
 */
LOG_EXPORT void log_producer_config_set_send_timeout_sec(log_producer_config * config, int32_t send_timeout_sec);

/**
 * set wait seconds when destroy flusher
 * @param config
 * @param destroy_flusher_wait_sec
 */
LOG_EXPORT void log_producer_config_set_destroy_flusher_wait_sec(log_producer_config * config, int32_t destroy_flusher_wait_sec);

/**
 * set wait seconds when destroy sender
 * @param config
 * @param destroy_sender_wait_sec
 */
LOG_EXPORT void log_producer_config_set_destroy_sender_wait_sec(log_producer_config * config, int32_t destroy_sender_wait_sec);

/**
 * set compress type, default 1 (lz4)
 * @param config
 * @param compress_type only support 1 or 0. 1 -> lz4, 0 -> no compress
 */
LOG_EXPORT void log_producer_config_set_compress_type(log_producer_config * config, int32_t compress_type);

/**
 * default http, 0 http, 1 https
 * @param config
 * @param using_https
 */
LOG_EXPORT void log_producer_config_set_using_http(log_producer_config * config, int32_t using_https);


/**
 * default http, 0 http, 1 https
 * @param config
 * @param log_queue_size
 */
LOG_EXPORT void log_producer_config_set_log_queue_size(log_producer_config * config, int32_t log_queue_size);



/**
 * destroy config, this will free all memory allocated by this config
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
