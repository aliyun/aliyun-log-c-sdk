//
// Created by ZhangCheng on 20/11/2017.
//

#ifndef LOG_C_SDK_LOG_PRODUCER_CLIENT_H
#define LOG_C_SDK_LOG_PRODUCER_CLIENT_H


#include "log_define.h"
#include "log_producer_config.h"
#include "apr_portable.h"
// @debug
#include "aos_log.h"
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

/**
 * init log producer environment
 * @note should been called before create any log producer client
 * @note no multi thread safe
 * @return OK if success, others the error code
 */
extern log_producer_result log_producer_env_init();

/**
 * destroy log producer environment
 * @note should been called after all log producer clients destroyed
 * @note no multi thread safe
 */
extern void log_producer_env_destroy();

/**
 * create log producer client with a producer config
 * @param config log_producer_config
 * @param send_done_function this function will be called when send done(can be ok or fail), set to NULL if you don't care about it
 * @return client ptr, NULL if create fail
 */
extern log_producer_client * create_log_producer_client(log_producer_config * config, on_log_producer_send_done_function send_done_function);

/**
 * create log producer client from config file
 * @param configFilePath config file path
 * @param send_done_function this function will be called when send done(can be ok or fail), set to NULL if you don't care about it
 * @return client ptr, NULL if create fail
 * @return
 */
extern log_producer_client * create_log_producer_client_by_config_file(const char * configFilePath, on_log_producer_send_done_function send_done_function);

/**
 * destroy log producer client
 * @param client
 * @note no multi thread safe
 */
extern void destroy_log_producer_client(log_producer_client * client);

/**
 * add log to producer, this may return LOG_PRODUCER_DROP_ERROR if buffer is full.
 * if you care about this log very much, retry when return LOG_PRODUCER_DROP_ERROR.
 *
 * @example  log_producer_client_add_log(client, 4, "key_1", "value_1", "key_2", "value_2")
 * @note log param ... must be const char * or char *
 * @note multi thread safe
 * @param client
 * @param kv_count key value count
 * @param ... log params : key, value pairs, must be const char * or char *
 * @return ok if success, LOG_PRODUCER_DROP_ERROR if fail.
 */
extern log_producer_result log_producer_client_add_log(log_producer_client * client, int32_t kv_count, ...);


/**
 * for LOG_PRODUCER_ADD_LOG
 */
#define LOG_PRODUCER_MARCO_EXPAND(...)                 __VA_ARGS__
#define LOG_PRODUCER_RSEQ_N() 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0
#define LOG_PRODUCER_ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, N, ...) N

#define LOG_PRODUCER_GET_ARG_COUNT_INNER(...)    LOG_PRODUCER_MARCO_EXPAND(LOG_PRODUCER_ARG_N(__VA_ARGS__))
#define LOG_PRODUCER_GET_ARG_COUNT(...)          LOG_PRODUCER_GET_ARG_COUNT_INNER(__VA_ARGS__, LOG_PRODUCER_RSEQ_N())

/**
 * macro for add log, this will set level、thread、file、line、function pairs to this log
 */
#define LOG_PRODUCER_ADD_LOG(client, level, ...)  \
    do                                      \
    {                                       \
        if (level < client->log_level) \
            break;  \
        int32_t n = LOG_PRODUCER_GET_ARG_COUNT(__VA_ARGS__) + 10; \
        char threadStr[12]; \
        int32_t threadNo = ((int32_t)apr_os_thread_current() & 0x7FFFFFF);\
        log_producer_atoi(threadStr, threadNo); \
        char lineStr[12]; \
        log_producer_atoi(lineStr, __LINE__); \
        log_producer_client_add_log(client, n, "_level_", log_producer_level_string[level], \
                                "_thread_", threadStr, "_file_", __FILE__, "_line_", lineStr, \
                                "_function_", __FUNCTION__, __VA_ARGS__);\
       \
    }while(0)

/**
 * macro for add log
 * @example LOG_PRODUCER_DEBUG(login_logger, "user", "jack", "user_id", "123456", "action", "login")
 * @note log params must be const char * or char *
 * @note multi thread safe
 */
#define LOG_PRODUCER_DEBUG(client, ...) LOG_PRODUCER_ADD_LOG(client, LOG_PRODUCER_LEVEL_DEBUG, __VA_ARGS__)
#define LOG_PRODUCER_INFO(client, ...) LOG_PRODUCER_ADD_LOG(client, LOG_PRODUCER_LEVEL_INFO, __VA_ARGS__)
#define LOG_PRODUCER_WARN(client, ...) LOG_PRODUCER_ADD_LOG(client, LOG_PRODUCER_LEVEL_WARN, __VA_ARGS__)
#define LOG_PRODUCER_ERROR(client, ...) LOG_PRODUCER_ADD_LOG(client, LOG_PRODUCER_LEVEL_ERROR, __VA_ARGS__)
#define LOG_PRODUCER_FATAL(client, ...) LOG_PRODUCER_ADD_LOG(client, LOG_PRODUCER_LEVEL_FATAL, __VA_ARGS__)


LOG_CPP_END

#endif //LOG_C_SDK_LOG_PRODUCER_CLIENT_H
