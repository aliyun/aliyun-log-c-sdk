//
// Created by ZhangCheng on 20/11/2017.
//

#ifndef LOG_C_SDK_LOG_PRODUCER_CLIENT_H
#define LOG_C_SDK_LOG_PRODUCER_CLIENT_H


#include "log_define.h"
#include "log_producer_config.h"
LOG_CPP_START


/**
 * log producer client
 */
typedef struct _log_producer_client
{
    volatile int32_t valid_flag;
    int32_t log_level;
    void * private_data;
}log_producer_client;

typedef struct _log_producer log_producer;

/**
 * init log producer environment
 * @note should been called before create any log producer client
 * @note no multi thread safe
 * @return OK if success, others the error code
 */
LOG_EXPORT log_producer_result log_producer_env_init();

/**
 * destroy log producer environment
 * @note should been called after all log producer clients destroyed
 * @note no multi thread safe
 */
LOG_EXPORT void log_producer_env_destroy();

/**
 * create log producer with a producer config
 * @param config log_producer_config
 * @param send_done_function this function will be called when send done(can be ok or fail), set to NULL if you don't care about it
 * @param user_param this param will send back in send_done_function
 * @return producer client ptr, NULL if create fail
 */
LOG_EXPORT log_producer * create_log_producer(log_producer_config * config, on_log_producer_send_done_function send_done_function, void *user_param);

/**
 * destroy log producer
 * @param producer
 * @note no multi thread safe
 */
LOG_EXPORT void destroy_log_producer(log_producer * producer);

/**
 * get client from producer
 * @param producer
 * @param config_name  useless now, set NULL
 * @return the specific producer client, root client if config_name is NULL or no specific config,
 */
LOG_EXPORT log_producer_client * get_log_producer_client(log_producer * producer, const char * config_name);


/**
 * force send data when network recover
 * @param client
 */
LOG_EXPORT void log_producer_client_network_recover(log_producer_client * client);

/**
 * add log to producer, this may return LOG_PRODUCER_DROP_ERROR if buffer is full.
 * if you care about this log very much, retry when return LOG_PRODUCER_DROP_ERROR.
 *
 * @example  log_producer_client_add_log(client, 4, "key_1", "value_1", "key_2", "value_2")
 * @note log param ... must be const char * or char * with '\0' ended
 * @note multi thread safe
 * @param client
 * @param kv_count key value count
 * @param ... log params : key, value pairs, must be const char * or char *
 * @return ok if success, LOG_PRODUCER_DROP_ERROR if buffer is full, LOG_PRODUCER_INVALID if client is destroyed.
 */
LOG_EXPORT log_producer_result log_producer_client_add_log(log_producer_client * client, int32_t kv_count, ...);

/**
 * add log to producer, this may return LOG_PRODUCER_DROP_ERROR if buffer is full.
 * if you care about this log very much, retry when return LOG_PRODUCER_DROP_ERROR.
 *
 * @param client
 * @param pair_count key value pair count
 * @note pair_count not kv_count
 * @param keys the key array
 * @param key_lens the key len array
 * @param values the value array
 * @param value_lens the value len array
 * @param flush if this log info need to send right, 1 mean flush and 0 means NO
 * @return ok if success, LOG_PRODUCER_DROP_ERROR if buffer is full, LOG_PRODUCER_INVALID if client is destroyed, LOG_PRODUCER_PERSISTENT_ERROR is save binlog failed.
 */
LOG_EXPORT log_producer_result log_producer_client_add_log_with_len(log_producer_client * client, int32_t pair_count, char ** keys, size_t * key_lens, char ** values, size_t * value_lens, int flush);

/**
 * @note same with log_producer_client_add_log_with_len but use int32_t as length
 * add log to producer, this may return LOG_PRODUCER_DROP_ERROR if buffer is full.
 * if you care about this log very much, retry when return LOG_PRODUCER_DROP_ERROR.
 *
 * @param client
 * @param pair_count key value pair count
 * @note pair_count not kv_count
 * @param keys the key array
 * @param key_lens the key len array
 * @param values the value array
 * @param value_lens the value len array
 * @param flush if this log info need to send right, 1 mean flush and 0 means NO
 * @return ok if success, LOG_PRODUCER_DROP_ERROR if buffer is full, LOG_PRODUCER_INVALID if client is destroyed, LOG_PRODUCER_PERSISTENT_ERROR is save binlog failed.
 */
LOG_EXPORT log_producer_result log_producer_client_add_log_with_len_int32(log_producer_client * client, int32_t pair_count, char ** keys, int32_t * key_lens, char ** values, int32_t * value_lens, int flush);

/**
 * @note same with log_producer_client_add_log_with_len_int32 but set time
 * add log to producer, this may return LOG_PRODUCER_DROP_ERROR if buffer is full.
 * if you care about this log very much, retry when return LOG_PRODUCER_DROP_ERROR.
 *
 * @param client
 * @param pair_count key value pair count
 * @note pair_count not kv_count
 * @param keys the key array
 * @param key_lens the key len array
 * @param values the value array
 * @param value_lens the value len array
 * @param flush if this log info need to send right, 1 mean flush and 0 means NO
 * @return ok if success, LOG_PRODUCER_DROP_ERROR if buffer is full, LOG_PRODUCER_INVALID if client is destroyed, LOG_PRODUCER_PERSISTENT_ERROR is save binlog failed.
 */
LOG_EXPORT log_producer_result log_producer_client_add_log_with_len_time_int32(log_producer_client * client, uint32_t time_sec, int32_t pair_count, char ** keys, int32_t * key_lens, char ** values, int32_t * value_lens, int flush);


/**
 * add raw pb log buffer
 * @param client
 * @param logBuf
 * @param logSize
 * @param flush
 * @return same as log_producer_client_add_log_with_len
 */
LOG_EXPORT log_producer_result log_producer_client_add_log_raw(log_producer_client * client,
                                                                 char * logBuf,
                                                                 size_t logSize,
                                                                 int flush);

/**
 * add raw log with string buffer
 * @param client
 * @param logTime
 * @param logItemCount
 * @param logItemsBuf
 * @param logItemsSize
 * @param flush
 * @return same as log_producer_client_add_log_with_len
 */
LOG_EXPORT log_producer_result log_producer_client_add_log_with_array(log_producer_client * client,
                                                             uint32_t logTime,
                                                             size_t  logItemCount,
                                                             const char * logItemsBuf,
                                                             const uint32_t * logItemsSize,
                                                             int flush);


/**
* add raw log buffer to client, this function is used to send buffers which can not send out when producer has destroyed
* @param client
* @param log_bytes
* @param compressed_bytes
* @param raw_buffer
* @return ok if success, LOG_PRODUCER_DROP_ERROR if buffer is full, LOG_PRODUCER_INVALID if client is destroyed, LOG_PRODUCER_PERSISTENT_ERROR is save binlog failed.
*/
LOG_EXPORT log_producer_result log_producer_client_add_raw_log_buffer(log_producer_client * client, size_t log_bytes, size_t compressed_bytes, const unsigned char * raw_buffer);

LOG_CPP_END

#endif //LOG_C_SDK_LOG_PRODUCER_CLIENT_H
