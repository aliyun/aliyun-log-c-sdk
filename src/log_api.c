#include "log_util.h"
#include "log_api.h"
#ifdef WIN32
#include "curl/curl.h"
#else
#include <curl/curl.h>
#endif
#include <string.h>
#include "sds.h"
#include "inner_log.h"
#include "log_producer_config.h"

#ifdef WIN32
#undef interface
#endif // WIN32

unsigned int LOG_GET_TIME();
void log_set_local_server_real_time(uint32_t serverTime);

log_status_t sls_log_init()
{
    CURLcode ecode;
    if ((ecode = curl_global_init(CURL_GLOBAL_ALL)) != CURLE_OK)
    {
        aos_error_log("curl_global_init failure, code:%d %s.\n", ecode, curl_easy_strerror(ecode));
        return -1;
    }
    return 0;
}
void sls_log_destroy()
{
    curl_global_cleanup();
}

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
    size_t totalLen = size * nmemb;
    //printf("body  ---->  %d  %s \n", (int) (totalLen), (const char*) ptr);
    sds * buffer = (sds *)stream;
    if (*buffer == NULL)
    {
        *buffer = sdsnewEmpty(256);
    }
    *buffer = sdscpylen(*buffer, ptr, totalLen);
    return totalLen;
}

static void processUnixTimeFromHeader(char * ptr, int size)
{
    char buf[64];
    memset(buf, 0, sizeof(buf));
    int i = 0;
    for (i = 0; i < size; ++i)
    {
        if (ptr[i] < '0' || ptr[i] > '9')
        {
            continue;
        }
        else
        {
            break;
        }
    }
    int j = 0;
    for (j = 0; j < 60 && i < size; ++j, ++i)
    {
        if (ptr[i] >= '0' && ptr[i] <= '9')
        {
            buf[j] = ptr[i];
        }
        else
        {
            break;
        }
    }
    long long serverTime = atoll(buf);
    if (serverTime <= 1500000000 || serverTime > 4294967294)
    {
        // invalid time
        return;
    }
    int deltaTime = serverTime - (long long)time(NULL);
    if (deltaTime > 30 || deltaTime < -30)
    {
        log_set_local_server_real_time(serverTime);
    }
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
    if (totalLen > 10 && memcmp(ptr, "x-log-time", 10) == 0)
    {
        processUnixTimeFromHeader((char *)ptr + 10, totalLen - 10);
    }
    return totalLen;
}

static const char sls_month_snames[12][4] =
        {
                "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
        };
static const char sls_day_snames[7][4] =
        {
                "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
        };

void sls_rfc822_date(char *date_str, struct tm * xt)
{
    const char *s = NULL;
    int real_year = 2000;


    /* example: "Sat, 08 Jan 2000 18:31:41 GMT" */
    /*           12345678901234567890123456789  */

    s = &sls_day_snames[xt->tm_wday][0];
    *date_str++ = *s++;
    *date_str++ = *s++;
    *date_str++ = *s++;
    *date_str++ = ',';
    *date_str++ = ' ';
    *date_str++ = xt->tm_mday / 10 + '0';
    *date_str++ = xt->tm_mday % 10 + '0';
    *date_str++ = ' ';
    s = &sls_month_snames[xt->tm_mon][0];
    *date_str++ = *s++;
    *date_str++ = *s++;
    *date_str++ = *s++;
    *date_str++ = ' ';
    real_year = 1900 + xt->tm_year;
    /* This routine isn't y10k ready. */
    *date_str++ = real_year / 1000 + '0';
    *date_str++ = real_year % 1000 / 100 + '0';
    *date_str++ = real_year % 100 / 10 + '0';
    *date_str++ = real_year % 10 + '0';
    *date_str++ = ' ';
    *date_str++ = xt->tm_hour / 10 + '0';
    *date_str++ = xt->tm_hour % 10 + '0';
    *date_str++ = ':';
    *date_str++ = xt->tm_min / 10 + '0';
    *date_str++ = xt->tm_min % 10 + '0';
    *date_str++ = ':';
    *date_str++ = xt->tm_sec / 10 + '0';
    *date_str++ = xt->tm_sec % 10 + '0';
    *date_str++ = ' ';
    *date_str++ = 'G';
    *date_str++ = 'M';
    *date_str++ = 'T';
    *date_str++ = 0;
    return;
}

void get_now_time_str(char * buffer, int bufLen, int timeOffset)
{
    time_t rawtime = LOG_GET_TIME();
    struct tm * timeinfo;
    if (timeOffset != 0)
    {
      rawtime += timeOffset;
    }
    timeinfo = gmtime(&rawtime);
    sls_rfc822_date(buffer, timeinfo);
}

void post_log_result_destroy(post_log_result * result)
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

void fetch_server_time_from_sls(log_producer_config * config)
{
    // must more than 1 count
    lz4_log_buf buf[2];
    buf->length = 2;
    buf->raw_length = 2;
    post_log_result * rst = post_logs_from_lz4buf(config->endpoint,
                                                  "slslogcsdksynctime",
                                                  "slslogcsdksynctime",
                                                  NULL,
                                                  config->endpoint,
                                                  "slslogcsdksynctime",
                                                  &buf[0],
                                                  NULL);
    post_log_result_destroy(rst);
}

#ifdef WIN32
DWORD WINAPI log_fetch_server_time_from_sls_thread(LPVOID param)
#else
void * log_fetch_server_time_from_sls_thread(void * param)
#endif
{
    log_producer_config * config = (log_producer_config *)param;
    fetch_server_time_from_sls(config);
    return NULL;
}

void async_fetch_server_time_from_sls(log_producer_config * config)
{
    THREAD thread;
    THREAD_INIT(thread, log_fetch_server_time_from_sls_thread, config);
}

post_log_result * post_logs_from_lz4buf(const char *endpoint, const char * accesskeyId, const char *accessKey, const char *stsToken, const char *project, const char *logstore, lz4_log_buf * buffer, log_post_option * option)
{
    post_log_result * result = (post_log_result *)malloc(sizeof(post_log_result));
    memset(result, 0, sizeof(post_log_result));
    CURL *curl = curl_easy_init();
    if (curl != NULL)
    {
        // url
        sds url = sdsnew("http://");
        url = sdscat(url, project);
        url = sdscat(url, ".");
        url = sdscat(url, endpoint);
        url = sdscat(url, "/logstores/");
        url = sdscat(url, logstore);
        url = sdscat(url, "/shards/lb");

        curl_easy_setopt(curl, CURLOPT_URL, url);

        char nowTime[64];
        int ntp_time_offset = option != NULL ? option->ntp_time_offset : 0;
        get_now_time_str(nowTime, 64, ntp_time_offset);

        char md5Buf[33];
        md5Buf[32] = '\0';
        int lz4Flag = option == NULL || option->compress_type == 1;
        md5_to_string((const char *)buffer->data, buffer->length, (char *)md5Buf);


        //puts(md5Buf);
        //puts(nowTime);

        struct curl_slist* headers = NULL;

        headers=curl_slist_append(headers, "Content-Type:application/x-protobuf");
        headers=curl_slist_append(headers, "x-log-apiversion:0.6.0");
        if (lz4Flag)
        {
          headers = curl_slist_append(headers, "x-log-compresstype:lz4");
        }
        if (stsToken != NULL)
        {
          sds tokenHeader = sdsnew("x-acs-security-token:");
          tokenHeader = sdscat(tokenHeader, stsToken);
          headers = curl_slist_append(headers, tokenHeader);
          sdsfree(tokenHeader);
        }
        headers=curl_slist_append(headers, "x-log-signaturemethod:hmac-sha1");
        sds headerTime = sdsnew("Date:");
        headerTime = sdscat(headerTime, nowTime);
        headers=curl_slist_append(headers, headerTime);
        sds headerMD5 = sdsnew("Content-MD5:");
        headerMD5 = sdscat(headerMD5, md5Buf);
        headers=curl_slist_append(headers, headerMD5);

        sds headerLen= sdsnewEmpty(64);
        headerLen = sdscatprintf(headerLen, "Content-Length:%d", (int)buffer->length);
        headers=curl_slist_append(headers, headerLen);

        sds headerRawLen = sdsnewEmpty(64);
        headerRawLen = sdscatprintf(headerRawLen, "x-log-bodyrawsize:%d", (int)buffer->raw_length);
        headers=curl_slist_append(headers, headerRawLen);

        sds headerHost = sdsnewEmpty(128);
        headerHost = sdscatprintf(headerHost, "Host:%s.%s", project, endpoint);
        headers=curl_slist_append(headers, headerHost);

        char sha1Buf[65];
        sha1Buf[64] = '\0';

        sds sigContent = sdsnewEmpty(512);
        if (lz4Flag)
        {
          if (stsToken == NULL)
          {
            sigContent = sdscatprintf(sigContent,
              "POST\n%s\napplication/x-protobuf\n%s\nx-log-apiversion:0.6.0\nx-log-bodyrawsize:%d\nx-log-compresstype:lz4\nx-log-signaturemethod:hmac-sha1\n/logstores/%s/shards/lb",
              md5Buf, nowTime, (int)buffer->raw_length, logstore);
          }
          else
          {
            sigContent = sdscatprintf(sigContent,
              "POST\n%s\napplication/x-protobuf\n%s\nx-acs-security-token:%s\nx-log-apiversion:0.6.0\nx-log-bodyrawsize:%d\nx-log-compresstype:lz4\nx-log-signaturemethod:hmac-sha1\n/logstores/%s/shards/lb",
              md5Buf, nowTime, stsToken, (int)buffer->raw_length, logstore);
          }
        }
        else
        {
          if (stsToken == NULL)
          {
            sigContent = sdscatprintf(sigContent,
              "POST\n%s\napplication/x-protobuf\n%s\nx-log-apiversion:0.6.0\nx-log-bodyrawsize:%d\nx-log-signaturemethod:hmac-sha1\n/logstores/%s/shards/lb",
              md5Buf, nowTime, (int)buffer->raw_length, logstore);
          }
          else
          {
            sigContent = sdscatprintf(sigContent,
              "POST\n%s\napplication/x-protobuf\n%s\nx-acs-security-token:%s\nx-log-apiversion:0.6.0\nx-log-bodyrawsize:%d\nx-log-signaturemethod:hmac-sha1\n/logstores/%s/shards/lb",
              md5Buf, nowTime, stsToken, (int)buffer->raw_length, logstore);
          }
        }

        //puts("#######################");
        //puts(sigContent);

        int destLen = signature_to_base64(sigContent, sdslen(sigContent), accessKey, strlen(accessKey), sha1Buf);
        sha1Buf[destLen] = '\0';
        //puts(sha1Buf);
        sds headerSig = sdsnewEmpty(256);
        headerSig = sdscatprintf(headerSig, "Authorization:LOG %s:%s", accesskeyId, sha1Buf);
        //puts(headerSig);
        headers=curl_slist_append(headers, headerSig);


        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POST, 1);

        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (void *)buffer->data);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, buffer->length);


        curl_easy_setopt(curl, CURLOPT_FILETIME, 1);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
        curl_easy_setopt(curl, CURLOPT_TCP_NODELAY, 1);
        curl_easy_setopt(curl, CURLOPT_NETRC, CURL_NETRC_IGNORED);

        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "log-c-lite_0.1.0");

        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15);

        if (option != NULL)
        {
          // interface
          if (option->interface != NULL)
          {
            curl_easy_setopt(curl, CURLOPT_INTERFACE, option->interface);
          }
          if (option->operation_timeout > 0)
          {
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, option->operation_timeout);
          }
          if (option->connect_timeout > 0)
          {
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, option->connect_timeout);
          }
        }

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
                result->statusCode = -2;
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
        sdsfree(url);
        sdsfree(headerTime);
        sdsfree(headerMD5);
        sdsfree(headerLen);
        sdsfree(headerRawLen);
        sdsfree(headerHost);
        sdsfree(sigContent);
        sdsfree(headerSig);
        /* always cleanup */
        curl_easy_cleanup(curl);
    }


    return result;
}
