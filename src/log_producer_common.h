//
// Created by ZhangCheng on 21/11/2017.
//

#ifndef LOG_C_SDK_LOG_PRODUCER_COMMON_H_H
#define LOG_C_SDK_LOG_PRODUCER_COMMON_H_H

#include "log_define.h"
LOG_CPP_START

/**
 * log producer level
 */
typedef int log_producer_level;

#define LOG_PRODUCER_LEVEL_NONE (0)
#define LOG_PRODUCER_LEVEL_DEBUG (1)
#define LOG_PRODUCER_LEVEL_INFO (2)
#define LOG_PRODUCER_LEVEL_WARN (3)
#define LOG_PRODUCER_LEVEL_ERROR (4)
#define LOG_PRODUCER_LEVEL_FATAL (5)
#define LOG_PRODUCER_LEVEL_MAX (6)

/**
 * log producer level string array, use this to cast level to string
 */
extern const char * log_producer_level_string[];

/**
 * log producer result for all operation
 */
typedef int log_producer_result;

/**
 * callback function for producer client
 * @param result send result
 * @param log_bytes log group packaged bytes
 * @param compressed_bytes lz4 compressed bytes
 * @param error_message if send result is not ok, error message is set. must check if is NULL when use it
 * @param raw_buffer lz4 buffer, you can call deserialize_from_lz4_proto_buf(raw_buffer, compressed_bytes, log_bytes) to get this loggroup
 * @note you can only read raw_buffer, but can't modify or free it; you must call log_group_free after deserialize_from_lz4_proto_buf
 */
typedef void (*on_log_producer_send_done_function)(const char * config_name, log_producer_result result, size_t log_bytes, size_t compressed_bytes, const char * req_id, const char * error_message, const unsigned char * raw_buffer);

extern log_producer_result LOG_PRODUCER_OK;
extern log_producer_result LOG_PRODUCER_INVALID;
extern log_producer_result LOG_PRODUCER_WRITE_ERROR;
extern log_producer_result LOG_PRODUCER_DROP_ERROR;
extern log_producer_result LOG_PRODUCER_SEND_NETWORK_ERROR;
extern log_producer_result LOG_PRODUCER_SEND_QUOTA_ERROR;
extern log_producer_result LOG_PRODUCER_SEND_UNAUTHORIZED;
extern log_producer_result LOG_PRODUCER_SEND_SERVER_ERROR;
extern log_producer_result LOG_PRODUCER_SEND_DISCARD_ERROR;
extern log_producer_result LOG_PRODUCER_SEND_TIME_ERROR;



typedef int log_producer_priority;

#define LOG_PRODUCER_PRIORITY_LOWEST (1)
#define LOG_PRODUCER_PRIORITY_LOW (2)
#define LOG_PRODUCER_PRIORITY_NORMAL (3)
#define LOG_PRODUCER_PRIORITY_HIGH (4)
#define LOG_PRODUCER_PRIORITY_HIGHEST (5)

/**
 * check if rst if ok
 * @param rst
 * @return 1 if ok, 0 not ok
 */
extern int is_log_producer_result_ok(log_producer_result rst);

/**
 * get string value of log producer operation result
 * @param rst
 * @return
 */
extern const char * get_log_producer_result_string(log_producer_result rst);

extern char * get_ip_by_host_name(apr_pool_t * pool);

extern void log_producer_atoi(char * buffer, int32_t v);

LOG_CPP_END

#endif //LOG_C_SDK_LOG_PRODUCER_COMMON_H_H
