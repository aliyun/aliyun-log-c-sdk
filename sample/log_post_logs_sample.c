#include "log_util.h"
#include "log_api.h"
#include "log_config.h"
#include <string.h>
#include <stdio.h>
#include <curl/curl.h>
#include "sds.h"
#include "log_util_imp.h"
#include <unistd.h>

// just for developer
// 本示例演示如何使用libcurl上传数据到日志服务

/**
 * 日志发送结果的封装
 */
struct _post_log_result
{
    int statusCode; // 发送结果，为负数时表示内部错误，statusCode/100 = 2时表示发送成功，否则为服务端返回的错误
    char * errorMessage; // 错误信息，可能为libcurl库返回的错误或服务端返回的错误
    char * requestID; // 服务端返回的 request id
};
typedef struct _post_log_result post_log_result;

/**
 * libcurl 的返回值 body回调函数
 * @param ptr
 * @param size
 * @param nmemb
 * @param stream
 * @return
 */
static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
    size_t totalLen = size * nmemb;
    sds * buffer = (sds *)stream;
    if (*buffer == NULL)
    {
        *buffer = sdsnewEmpty(256);
    }
    *buffer = sdscpylen(*buffer, ptr, totalLen);
    return totalLen;
}

/**
 * libcurl 的返回值 header回调函数
 * @param ptr
 * @param size
 * @param nmemb
 * @param stream
 * @return
 */
static size_t header_callback(void *ptr, size_t size, size_t nmemb, void *stream)
{
    size_t totalLen = size * nmemb;
    sds * buffer = (sds *)stream;
    // only copy header start with x-log-
    if (totalLen > 6 && memcmp(ptr, "x-log-", 6) == 0)
    {
        *buffer = sdscpylen(*buffer, ptr, totalLen);
    }
    return totalLen;
}

/**
 * 释放发送结果
 * @param result
 */
void free_post_log_result(post_log_result * result)
{
    if (result != NULL)
    {
        if (result->errorMessage != NULL)
        {
            sdsfree(result->errorMessage);
        }
        if (result->requestID != NULL)
        {
            sdsfree(result->requestID);
        }
        free(result);
    }
}

int should_fix_time(post_log_result * result)
{
    if (result != NULL && result->errorMessage != NULL)
    {
        return strstr(result->errorMessage, LOGE_TIME_EXPIRED) != NULL;
    }
    return 0;
}

int should_drop_data(post_log_result * result)
{
    return result != NULL && (result->statusCode == 401 || result->statusCode == 404);
}

// 是否使用lz4的宏定义，默认关，需要使用时cmake时加入 -DUSE_LZ4=ON
#ifdef USE_LZ4_FLAG

/**
 * 使用libcurl发送lz4后的数据
 * @param bder
 * @return
 */
post_log_result * post_logs_with_libcurl_lz4(log_group_builder * bder)
{
    post_log_result * result = (post_log_result *)malloc(sizeof(post_log_result));
    memset(result, 0, sizeof(post_log_result));
    result->statusCode = -1;
    lz4_log_buf* lz4LogBuf = serialize_to_proto_buf_with_malloc_lz4(bder);
    if (lz4LogBuf != NULL)
    {

        result->statusCode = -2;
        post_log_header log_header = pack_logs_from_buffer_lz4(LOG_ENDPOINT, ACCESS_KEY_ID, ACCESS_KEY_SECRET,
                                                               PROJECT_NAME, LOGSTORE_NAME,
                                                               (const char *)lz4LogBuf->data, lz4LogBuf->length, lz4LogBuf->raw_length);

        if (log_header.header_items != NULL)
        {
            result->statusCode = -3;
            CURL *curl = curl_easy_init();
            if (curl != NULL)
            {

                sds url_total = sdsnew(log_header.url);
                url_total = sdscat(url_total, log_header.path);
                struct curl_slist* headers = NULL;
                uint8_t index = 0;
                for (index = 0; index < log_header.item_size; ++index)
                {
                    headers=curl_slist_append(headers, log_header.header_items[index]);
                }
                curl_easy_setopt(curl, CURLOPT_URL, url_total);

                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
                curl_easy_setopt(curl, CURLOPT_POST, 1);

                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (void *)lz4LogBuf->data);
                curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, lz4LogBuf->length);


                curl_easy_setopt(curl, CURLOPT_FILETIME, 1);
                curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
                curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
                curl_easy_setopt(curl, CURLOPT_TCP_NODELAY, 1);
                curl_easy_setopt(curl, CURLOPT_NETRC, CURL_NETRC_IGNORED);

                curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
                curl_easy_setopt(curl, CURLOPT_USERAGENT, "log-c-tiny_0.1.0");

                curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15);


                sds header = sdsnewEmpty(64);
                sds body = NULL;


                curl_easy_setopt(curl, CURLOPT_HEADERDATA, &header);
                curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);

                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);

                //curl_easy_setopt(curl,CURLOPT_VERBOSE,1); //打印调试信息

                CURLcode res = curl_easy_perform(curl);


                //printf("result : %s \n", curl_easy_strerror(res));
                long http_code;
                if (res == CURLE_OK)
                {
                    if ((res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code)) != CURLE_OK)
                    {
                        printf("get info result : %s \n", curl_easy_strerror(res));
                        result->statusCode = -4;
                    } else {
                        result->statusCode = http_code;
                    }
                }
                else
                {
                    if (body == NULL)
                    {
                        body = sdsnew(curl_easy_strerror(res));
                    }
                    else
                    {
                        body = sdscpy(body, curl_easy_strerror(res));
                    }
                    result->statusCode = -1 * (int)res;
                }
                // header and body 's pointer may be modified in callback (size > 256)
                if (sdslen(header) > 0)
                {
                    result->requestID = header;
                }
                else
                {
                    sdsfree(header);
                    header = NULL;
                }
                // body will be NULL or a error string(net error or request error)
                result->errorMessage = body;

                curl_slist_free_all(headers); /* free the list again */
                sdsfree(url_total);
                curl_easy_cleanup(curl);
            }
        }
        free_post_log_header(log_header);

        free_lz4_log_buf(lz4LogBuf);
    }
    return result;
}

#endif

/**
 * 使用libcurl发送未压缩的数据
 * @param bder
 * @return
 */
post_log_result * post_logs_with_libcurl(log_group_builder * bder)
{
    post_log_result * result = (post_log_result *)malloc(sizeof(post_log_result));
    memset(result, 0, sizeof(post_log_result));
    result->statusCode = -1;
    log_buf logBuf = serialize_to_proto_buf_with_malloc(bder);
    if (logBuf.buffer != NULL)
    {

        result->statusCode = -2;
        post_log_header log_header = pack_logs_from_raw_buffer(LOG_ENDPOINT, ACCESS_KEY_ID, ACCESS_KEY_SECRET,
                                                               PROJECT_NAME, LOGSTORE_NAME,
                                                               (const char *)logBuf.buffer, logBuf.n_buffer);

        if (log_header.header_items != NULL)
        {
            result->statusCode = -3;
            CURL *curl = curl_easy_init();
            if (curl != NULL)
            {

                sds url_total = sdsnew(log_header.url);
                url_total = sdscat(url_total, log_header.path);
                struct curl_slist* headers = NULL;
                uint8_t index = 0;
                for (index = 0; index < log_header.item_size; ++index)
                {
                    headers=curl_slist_append(headers, log_header.header_items[index]);
                }
                curl_easy_setopt(curl, CURLOPT_URL, url_total);

                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
                curl_easy_setopt(curl, CURLOPT_POST, 1);

                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (void *)logBuf.buffer);
                curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, logBuf.n_buffer);


                curl_easy_setopt(curl, CURLOPT_FILETIME, 1);
                curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
                curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
                curl_easy_setopt(curl, CURLOPT_TCP_NODELAY, 1);
                curl_easy_setopt(curl, CURLOPT_NETRC, CURL_NETRC_IGNORED);

                curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
                curl_easy_setopt(curl, CURLOPT_USERAGENT, "log-c-tiny_0.1.0");

                curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15);


                sds header = sdsnewEmpty(64);
                sds body = NULL;


                curl_easy_setopt(curl, CURLOPT_HEADERDATA, &header);
                curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);

                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);

                //curl_easy_setopt(curl,CURLOPT_VERBOSE,1); //打印调试信息

                CURLcode res = curl_easy_perform(curl);


                //printf("result : %s \n", curl_easy_strerror(res));
                long http_code;
                if (res == CURLE_OK)
                {
                    if ((res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code)) != CURLE_OK)
                    {
                        printf("get info result : %s \n", curl_easy_strerror(res));
                        result->statusCode = -4;
                    } else {
                        result->statusCode = http_code;
                    }
                }
                else
                {
                    if (body == NULL)
                    {
                        body = sdsnew(curl_easy_strerror(res));
                    }
                    else
                    {
                        body = sdscpy(body, curl_easy_strerror(res));
                    }
                    result->statusCode = -1 * (int)res;
                }
                // header and body 's pointer may be modified in callback (size > 256)
                if (sdslen(header) > 0)
                {
                    result->requestID = header;
                }
                else
                {
                    sdsfree(header);
                    header = NULL;
                }
                // body will be NULL or a error string(net error or request error)
                result->errorMessage = body;


                curl_slist_free_all(headers); /* free the list again */
                sdsfree(url_total);
                curl_easy_cleanup(curl);
            }
        }
        free_post_log_header(log_header);
    }
    return result;
}


#define POST_LOG_COUNT 10
#define POST_LOG_CONTENT_PAIRS 15

/**
 * 发送数据示例
 */
void post_logs_sample()
{
    int i = 0;
    for (i = 0; i < 10; ++i)
    {
        // 初始化 builder
        log_group_builder *bder = log_group_create();
        // 设置日志源
        add_source(bder, "10.230.201.117", sizeof("10.230.201.117"));
        // 设置topic
        add_topic(bder, "post_logs_sample",
                  sizeof("post_logs_sample"));

        // 设置日志的tag信息
        add_tag(bder, "taga_key", strlen("taga_key"), "taga_value",
                strlen("taga_value"));
        add_tag(bder, "tagb_key", strlen("tagb_key"), "tagb_value",
                strlen("tagb_value"));
        // 设置日志的packid（通常使用64位int的hex编码），同一个packid的数据处于同一个上下文，序列号从0开始单调递增
        add_pack_id(bder, "123456789ABC", strlen("123456789ABC"), i);

        int x;
        for (x = 0; x < POST_LOG_COUNT; x++)
        {
            // 构造日志的keys values以及key_lens val_lens
            char * keys[POST_LOG_CONTENT_PAIRS];
            char * values[POST_LOG_CONTENT_PAIRS];
            size_t key_lens[POST_LOG_CONTENT_PAIRS];
            size_t val_lens[POST_LOG_CONTENT_PAIRS];
            int j = 0;
            for (j = 0; j < POST_LOG_CONTENT_PAIRS; j++)
            {

                keys[j] = (char *)malloc(64);
                sprintf(keys[j], "key_%d", x * j);
                values[j] = (char *)malloc(64);
                sprintf(values[j], "value_%d", 2 * j);
                key_lens[j] = strlen(keys[j]);
                val_lens[j] = strlen(values[j]);
            }
            // 调用接口记录日志
            add_log_full(bder, time(NULL), POST_LOG_CONTENT_PAIRS, keys, key_lens, values, val_lens);
            for (j = 0; j < POST_LOG_CONTENT_PAIRS; j++)
            {
                free(keys[j]);
                free(values[j]);
            }
        }

        // if you want to use this functions, add `-DADD_LOG_KEY_VALUE_FUN=ON` for cmake. eg `cmake . -DADD_LOG_KEY_VALUE_FUN=ON`
#ifdef LOG_KEY_VALUE_FLAG
        for (x = 0; x < POST_LOG_COUNT; x++)
        {
            int j = 0;
            // 使用迭代的方式记录日志
            add_log_begin(bder);
            for (j = 0; j < POST_LOG_CONTENT_PAIRS; j++)
            {
                char key[64];
                sprintf(key, "add_key_%d", x * j);
                char value[64];
                sprintf(value, "add_value_%d", 2 * j);
                add_log_key_value(bder, key, strlen(key), value, strlen(value));
                if (j == POST_LOG_CONTENT_PAIRS / 2)
                {
                    add_log_time(bder, time(NULL));
                }
            }

            add_log_end(bder);
        }

#endif

        int sleepTimeSec = 2;
        do
        {
            // 使用本示例封装的libcurl接口发送日志
            post_log_result * result = post_logs_with_libcurl(bder);
            printf("post_logs_with_libcurl result code : %d, request id : %s, error : %s \n", result->statusCode, result->requestID, result->errorMessage);


            // 以下是对于日志发送失败建议的处理方式
            // not status 2xx
            // 如果发送失败，需要查看发送失败的原因
            if (result->statusCode / 100 != 2)
            {
                if (should_fix_time(result))
                {
                    // 如果是因为本地时间导致发送失败（本地因为时间没有校时和标准时间相差超过15分钟），用当前时间修正日志时间并重新发送

                    // 此处您的应用应尝试去进行主动校时
                    // sync_local_time()....

                    // 用当前时间修正日志时间
                    fix_log_group_time(bder->grp->logs.buffer, bder->grp->logs.now_buffer_len, time(NULL));
                }
                else if (should_drop_data(result))
                {
                    // 如果是因为鉴权失败，需要丢弃该条数据
                    free_post_log_result(result);
                    break;
                }
                else
                {
                    // 其他情况使用一套简单的退火算法重试
                    sleepTimeSec *= 2;
                    if (sleepTimeSec > 300)
                    {
                        sleepTimeSec = 300;
                    }
                    printf("we will try after %d seconds \n", sleepTimeSec);
                    sleep(sleepTimeSec);
                    printf("sleep done. \n");
                }
            }
            else
            {
                free_post_log_result(result);
                break;
            }


        }while (1);


#ifdef USE_LZ4_FLAG
        // 使用带lz4压缩的方式发送日志（通常压缩比为5-10倍）
        // @note 您也应该像post_logs_with_libcurl一样进行错误处理
        post_log_result * result = post_logs_with_libcurl_lz4(bder);
        printf("post_logs_with_libcurl_lz4 result code : %d, request id : %s, error : %s \n", result->statusCode, result->requestID, result->errorMessage);
        free_post_log_result(result);
#endif

        log_group_destroy(bder);
    }

}

int main(int argc, char *argv[])
{
    // for developer

    // 初始化日志发送的工具函数
    g_log_get_now_time_str_fun = get_now_time_str;
    g_log_hmac_sha1_signature_to_base64_fun = signature_to_base64;
    g_log_md5_to_string_fun = md5_to_string;

    // 初始化libcurl
    curl_global_init(CURL_GLOBAL_NOTHING);
    int i = 0;
    for (i = 0; i < 1; ++i)
    {
        // 发送日志
        post_logs_sample();
    }

    // clean up
    //curl_global_cleanup();
    return 0;
}
