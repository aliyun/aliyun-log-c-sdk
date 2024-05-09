#include "log_define.h"
#include <curl/curl.h>
#include "inner_log.h"
#include "sds.h"

typedef int log_curl_error_code;

log_curl_error_code log_curl_init_err = -1;
log_curl_error_code log_curl_getinfo_err = -2;
log_curl_error_code log_curl_perform_err = -3;

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
    size_t totalLen = size * nmemb;
    //printf("body  ---->  %d  %s \n", (int) (totalLen, (const char*) ptr);
    sds * buffer = (sds *)stream;
    if (*buffer == NULL)
    {
        *buffer = sdsnewEmpty(256);
    }
    *buffer = sdscpylen(*buffer, ptr, totalLen);
    return totalLen;
}

static size_t header_callback(void *ptr, size_t size, size_t nmemb, void *stream)
{
    size_t totalLen = size * nmemb;
    //printf("header  ---->  %d  %s \n", (int) (totalLen), (const char*) ptr);
    sds * buffer = (sds *)stream;
    // only copy header start with x-log-
    if (totalLen > 6 && memcmp(ptr, "x-log-", 6) == 0)
    {
        *buffer = sdscpylen(*buffer, ptr, totalLen);
    }
    // todo: handle server time
    return totalLen;
}

int log_curl_http_post(const char *url,
                    char **header_array,
                    int header_count,
                    const void *data,
                    int data_len)
{
    CURL *curl = curl_easy_init();
    if (curl == NULL)
    {
        aos_error_log("curl_easy_init failure, url:%s.\n", url);
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
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "log-c-lite_0.1.0");

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
            aos_error_log("get info result : %s \n", curl_easy_strerror(res));
            http_code = log_curl_getinfo_err;
        }
    }
    else
    {
        aos_error_log("curl_easy_perform failure : %s \n", curl_easy_strerror(res));
        resp_body = sdscpy(resp_body, curl_easy_strerror(res));
        http_code = log_curl_perform_err;
    }
    // todo: delete this
    aos_info_log("log_curl_http_post success: http_code=%ld, header=%s, body=%s.\n",
                    http_code, 
                    resp_header,
                    resp_body);

    if (http_code != 200)
    {
        aos_error_log("log_curl_http_post failure: http_code=%ld, header=%s, body=%s.\n",
                        http_code, 
                        resp_header,
                        resp_body);
    }

    sdsfree(resp_header);
    sdsfree(resp_body);
    curl_easy_cleanup(curl);
    return http_code;
}

log_status_t log_curl_global_init()
{
    CURLcode ecode;
    if ((ecode = curl_global_init(CURL_GLOBAL_ALL)) != CURLE_OK)
    {
        aos_error_log("curl_global_init failure, code:%d %s.\n", ecode, curl_easy_strerror(ecode));
        return -1;
    }
}

void log_curl_global_destroy()
{
    curl_global_cleanup();
}
