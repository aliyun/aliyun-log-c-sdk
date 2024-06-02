#include "adapter.h"
#include "log_define.h"
#include <curl/curl.h>
#include "inner_log.h"
#include "sds.h"
#include "log_http_interface.h"

typedef int log_curl_error_code;

static log_curl_error_code log_curl_init_err = -1;
static log_curl_error_code log_curl_getinfo_err = -2;
static log_curl_error_code log_curl_perform_err = -3;

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
    size_t totalLen = size * nmemb;
    sds *buffer = (sds *)stream;
    if (*buffer == NULL)
    {
        *buffer = sdsnewEmpty(256);
    }
    *buffer = sdscpylen(*buffer, ptr, totalLen);
    return totalLen;
}

static size_t header_callback(void *ptr, size_t size, size_t nmemb,
                              void *stream)
{
    size_t totalLen = size * nmemb;
    sds *buffer = (sds *)stream;
    if (totalLen > 6 && memcmp(ptr, "x-log-", 6) == 0)
    {
        *buffer = sdscpylen(*buffer, ptr, totalLen);
    }
    return totalLen;
}

static int log_curl_http_post(const char *url, char **header_array,
                              int header_count, const void *data, int data_len)
{
    CURL *curl = curl_easy_init();
    if (curl == NULL)
    {
        aos_error_log("curl_easy_init failure, url:%s", url);
        return log_curl_init_err;
    }
    curl_easy_setopt(curl, CURLOPT_URL, url);
    struct curl_slist* headers = NULL;
    for (int i = 0; i < header_count; i++)
    {
        headers = curl_slist_append(headers, header_array[i]);
    }

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POST, 1);

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data_len);

    curl_easy_setopt(curl, CURLOPT_FILETIME, 1);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
    curl_easy_setopt(curl, CURLOPT_TCP_NODELAY, 1);
    curl_easy_setopt(curl, CURLOPT_NETRC, CURL_NETRC_IGNORED);

    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "log-c-persistent_0.1.0");

    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15);


    sds resp_header = sdsnewEmpty(64);
    sds resp_body = NULL;
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &resp_header);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp_body);

    CURLcode res = curl_easy_perform(curl);
    long http_code = 200;
    if (resp_body == NULL)
    {
        resp_body = sdsnewEmpty(128);
    }

    if (res == CURLE_OK)
    {
        if ((res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code)) != CURLE_OK)
        {
            aos_error_log("get info result: %s", curl_easy_strerror(res));
            http_code = log_curl_getinfo_err;
        }
    }
    else
    {
        aos_error_log("curl_easy_perform failure: %s", curl_easy_strerror(res));
        resp_body = sdscpy(resp_body, curl_easy_strerror(res));
        http_code = log_curl_perform_err;
    }

    aos_debug_log("log_curl_http_post: http_code=%ld, header=%s, body=%s",
                  http_code, resp_header, resp_body);

    if (http_code != 200)
    {
      aos_error_log(
          "log_curl_http_post failed: http_code=%ld, header=%s, body=%s",
          http_code, resp_header, resp_body);
    }

    sdsfree(resp_header);
    sdsfree(resp_body);
    curl_easy_cleanup(curl);
    return http_code;
}

static log_status_t log_curl_global_init()
{
    CURLcode ecode;
    if ((ecode = curl_global_init(CURL_GLOBAL_ALL)) != CURLE_OK)
    {
        aos_error_log("curl_global_init failed, code:%d, err: %s", ecode, curl_easy_strerror(ecode));
        return -1;
    }
}

static void log_curl_global_destroy()
{
    curl_global_cleanup();
}

void log_set_http_use_curl()
{
    log_set_http_post_func(log_curl_http_post);
    log_set_http_global_init_func(log_curl_global_init);
    log_set_http_global_destroy_func(log_curl_global_destroy);
}