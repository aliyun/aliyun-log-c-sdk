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
    CRITICALSECTION securityTokenLock;
    log_producer_config_tag * tags;
    int32_t tagAllocSize;
    int32_t tagCount;

    int32_t sendThreadCount;

    int32_t packageTimeoutInMS;
    int32_t logCountPerPackage;
    int32_t logBytesPerPackage;
    int32_t maxBufferBytes;

    char * netInterface;
    int32_t connectTimeoutSec;
    int32_t sendTimeoutSec;
    int32_t destroyFlusherWaitTimeoutSec;
    int32_t destroySenderWaitTimeoutSec;

    int32_t compressType; // 0 no compress, 1 lz4
    int32_t ntpTimeOffset;
    int using_https; // 0 http, 1 https

    // for persistent feature
    int32_t usePersistent; // 0 no, 1 yes
    char * persistentFilePath;
    int32_t maxPersistentLogCount;
    int32_t maxPersistentFileSize; // max persistent size is maxPersistentFileSize * maxPersistentFileCount
    int32_t maxPersistentFileCount; // max persistent size is maxPersistentFileSize * maxPersistentFileCount
    int32_t forceFlushDisk; // force flush disk

    // if log's time if before than nowTime, it's time will be rewrite to nowTime if delta > maxLogDelayTime
    // the default value is 7*24*3600
    int32_t maxLogDelayTime;
    int32_t dropDelayLog; // 1 true, 0 false, default 1

    int32_t dropUnauthorizedLog; // 1 true, 0 false, default 0
    int32_t callbackFromSenderThread; // 1 true, 0 false, default 1int32_t callbackFromSenderThread;
    int32_t webTracking; // 1 webtracking, default 0

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
 * default http, 0 http, 1 https
 * @param config
 * @param using_https
 */
LOG_EXPORT void log_producer_config_set_using_http(log_producer_config * config, int32_t using_https);

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
* set time offset between local time and server ntp time
* @param config
* @param ntp_time_diff
*/
LOG_EXPORT void log_producer_config_set_ntp_time_offset(log_producer_config * config, int32_t ntp_time_offset);

/**
 * set persistent flag, 0 disable, 1 enable
 * @param config
 * @param persistent
 */
LOG_EXPORT void log_producer_config_set_persistent(log_producer_config * config, int32_t persistent);

/**
 * set persistent file path
 * @param config
 * @param file_path full file path, eg : "/app/test/data.dat"
 * @note the file dir must exist
 */
LOG_EXPORT void log_producer_config_set_persistent_file_path(log_producer_config * config, const char * file_path);

/**
 * set max persistent file count, max size in disk is file_count * file_size
 * @param config
 * @param file_count 1-1000
 */
LOG_EXPORT void log_producer_config_set_persistent_max_file_count(log_producer_config * config, int32_t file_count);

/**
 * set max persistent file size, max size in disk is file_count * file_size
 * @param config
 * @param file_size
 */
LOG_EXPORT void log_producer_config_set_persistent_max_file_size(log_producer_config * config, int32_t file_size);

/**
 * force flush disk when add a log, 0 disable, 1 enable
 * @param config
 * @param force
 */
LOG_EXPORT void log_producer_config_set_persistent_force_flush(log_producer_config * config, int32_t force);

/**
 * set max log count saved in disk
 * @param config
 * @param max_log_count
 */
LOG_EXPORT void log_producer_config_set_persistent_max_log_count(log_producer_config * config, int32_t max_log_count);

/**
 * set max log delay time
 * @param config
 * @param max_log_delay_time
 */
LOG_EXPORT void log_producer_config_set_max_log_delay_time(log_producer_config * config, int32_t max_log_delay_time);

/**
 * set drop delay log or not
 * @param config
 * @param drop_or_rewrite
 */
LOG_EXPORT void log_producer_config_set_drop_delay_log(log_producer_config * config, int32_t drop_or_rewrite);


/**
 * set drop unauthorized log or not
 * @param config
 * @param drop_or_not
 */
LOG_EXPORT void log_producer_config_set_drop_unauthorized_log(log_producer_config * config, int32_t drop_or_not);

/**
 * set callback thread. default from sender thread.
 * @param config
 * @param callback_from_sender_thread
 */
LOG_EXPORT void log_producer_config_set_callback_from_sender_thread(log_producer_config * config, int32_t callback_from_sender_thread);

LOG_EXPORT void log_producer_config_set_use_webtracking(log_producer_config * config, int32_t webtracking);

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

/**
 *
 * @return
 */
LOG_EXPORT int log_producer_persistent_config_is_enabled(log_producer_config * config);


LOG_CPP_END

#endif //LOG_C_SDK_LOG_PRODUCER_CONFIG_H
