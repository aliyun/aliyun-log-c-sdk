#include "log_util.h"
#include "log_api.h"
#include <curl/curl.h>
#include <string.h>
#include "sds.h"
#include "inner_log.h"

log_status_t sls_log_init(int32_t log_global_flag)
{
    CURLcode ecode;
    if ((ecode = curl_global_init(log_global_flag)) != CURLE_OK)
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
    return totalLen;
}

void get_now_time_str(char * buffer, int bufLen)
{
    time_t rawtime;
    struct tm * timeinfo;
    time (&rawtime);
    timeinfo = gmtime(&rawtime);
    strftime(buffer, bufLen, "%a, %d %b %Y %H:%M:%S GMT", timeinfo);
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

post_log_result * post_logs_from_lz4buf(const char *endpoint, const char * accesskeyId, const char *accessKey, const char *stsToken, const char *project, const char *logstore, lz4_log_buf * buffer, log_post_option * option)
{
    post_log_result * result = (post_log_result *)malloc(sizeof(post_log_result));
    memset(result, 0, sizeof(post_log_result));
    CURL *curl = curl_easy_init();
    if (curl != NULL)
    {
        // url
        sds url = NULL;
        if (option->using_https) {
            url = sdsnew("https://");
        } else {
            url = sdsnew("http://");
        }

        url = sdscat(url, project);
        url = sdscat(url, ".");
        url = sdscat(url, endpoint);
        url = sdscat(url, "/logstores/");
        url = sdscat(url, logstore);
        url = sdscat(url, "/shards/lb");

        curl_easy_setopt(curl, CURLOPT_URL, url);
        struct curl_slist *connect_to = NULL;
        if (option->remote_address != NULL)
        {
            // example.com::192.168.1.5:
            sds connect_to_item = sdsnew(project);
            connect_to_item = sdscat(connect_to_item, ".");
            connect_to_item = sdscat(connect_to_item, endpoint);
            connect_to_item = sdscat(connect_to_item, "::");
            connect_to_item = sdscat(connect_to_item, option->remote_address);
            connect_to_item = sdscat(connect_to_item, ":");

            connect_to = curl_slist_append(NULL, connect_to_item);
            curl_easy_setopt(curl, CURLOPT_CONNECT_TO, connect_to);
            sdsfree(connect_to_item);
        }

        char nowTime[64];
        get_now_time_str(nowTime, 64);

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
            headers=curl_slist_append(headers, "x-log-compresstype:lz4");
        }
        if (stsToken != NULL)
        {
            sds tokenHeader = sdsnew("x-acs-security-token:");
            tokenHeader = sdscat(tokenHeader, stsToken);
            headers=curl_slist_append(headers, tokenHeader);
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
        if (connect_to != NULL)
        {
            curl_slist_free_all(connect_to);
        }
    }


    return result;
}
