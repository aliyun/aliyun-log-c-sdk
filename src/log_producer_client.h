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
 * @param log_global_flag global log config flag
 * @note should been called before create any log producer client
 * @note no multi thread safe
 * @return OK if success, others the error code
 */
LOG_EXPORT log_producer_result log_producer_env_init(int32_t log_global_flag);

/**
 * create global send thread pool
 *
 * @note if producer have no send thread, use global send thread pool to send logs
 * @note this thread pool will been destroyed when you call log_producer_env_destroy
 * @note not thread safe
 *
 * @param log_global_send_thread_count
 * @param log_global_send_queue_size recommend values : for server apps, set 10000; for client apps, set 1000; for iot devices, set 100
 * @return
 */
LOG_EXPORT log_producer_result log_producer_global_send_thread_init(int32_t log_global_send_thread_count, int32_t log_global_send_queue_size);

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
 * @return producer client ptr, NULL if create fail
 */
LOG_EXPORT log_producer * create_log_producer(log_producer_config * config, on_log_producer_send_done_function send_done_function);

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
 * @return ok if success, LOG_PRODUCER_DROP_ERROR if buffer is full, LOG_PRODUCER_INVALID if client is destroyed.
 */
LOG_EXPORT log_producer_result log_producer_client_add_log_with_len(log_producer_client * client, int32_t pair_count, char ** keys, size_t * key_lens, char ** values, size_t * value_lens);


/**
 * add raw log buffer to client, this function is used to send buffers which can not send out when producer has destroyed
 * @param client
 * @param log_bytes
 * @param compressed_bytes
 * @param raw_buffer
 * @return ok if success, LOG_PRODUCER_DROP_ERROR if buffer is full, LOG_PRODUCER_INVALID if client is destroyed.
 */
LOG_EXPORT log_producer_result log_producer_client_add_raw_log_buffer(log_producer_client * client, size_t log_bytes, size_t compressed_bytes, const unsigned char * raw_buffer);

LOG_CPP_END

#endif //LOG_C_SDK_LOG_PRODUCER_CLIENT_H
