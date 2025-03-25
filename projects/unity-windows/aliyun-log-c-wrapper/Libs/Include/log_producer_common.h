//
// Created by ZhangCheng on 21/11/2017.
//

#ifndef LOG_C_SDK_LOG_PRODUCER_COMMON_H_H
#define LOG_C_SDK_LOG_PRODUCER_COMMON_H_H

#include "log_define.h"
#include <stdint.h>
#include <stddef.h>
LOG_CPP_START


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
 * @param raw_buffer lz4 buffer
 * @note you can only read raw_buffer, but can't modify or free it
 */
typedef void(*on_log_producer_send_done_function)(const char * config_name, log_producer_result result, size_t log_bytes, size_t compressed_bytes, const char * req_id, const char * error_message, const unsigned char * raw_buffer, void *user_param);

typedef void(*on_log_producer_send_done_uuid_function)(const char * config_name,
        log_producer_result result,
        size_t log_bytes,
        size_t compressed_bytes,
        const char * req_id,
        const char * error_message,
        const unsigned char * raw_buffer,
        void *user_param,
        int64_t startId,
        int64_t endId);


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
extern log_producer_result LOG_PRODUCER_SEND_EXIT_BUFFERED;
extern log_producer_result LOG_PRODUCER_PARAMETERS_INVALID;
extern log_producer_result LOG_PRODUCER_PERSISTENT_ERROR;

/**
 * check if rst if ok
 * @param rst
 * @return 1 if ok, 0 not ok
 */
LOG_EXPORT int is_log_producer_result_ok(log_producer_result rst);


LOG_CPP_END

#endif //LOG_C_SDK_LOG_PRODUCER_COMMON_H_H
