#include "log_util.h"
#include "log_api.h"
#include <curl/curl.h>
#include <string.h>
#include "sds.h"
#include "inner_log.h"
#include <stdio.h>
#include <time.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>

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

void get_now_time_str(char * buffer, int bufLen)
{
    time_t rawtime;
    struct tm timeinfo;
    time (&rawtime);
    gmtime_r(&rawtime, &timeinfo);
    sls_rfc822_date(buffer, &timeinfo);
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

void buildSignKey(const char *key, const char *region, const char *date, const char *stringToSign, char *result) {
    unsigned char sign_key[HMAC_MAX_MD_CBLOCK];
    unsigned char sign_date[HMAC_MAX_MD_CBLOCK];
    unsigned char sign_region[HMAC_MAX_MD_CBLOCK];
    unsigned char sign_service[HMAC_MAX_MD_CBLOCK];
    unsigned char sign_request[HMAC_MAX_MD_CBLOCK];
    unsigned int len;

    snprintf((char *)sign_key, sizeof(sign_key), "aliyun_v4%s", key);
    HMAC(EVP_sha256(), sign_key, strlen((char *)sign_key), (const unsigned char *)date, strlen(date), sign_date, &len);
    HMAC(EVP_sha256(), sign_date, len, (const unsigned char *)region, strlen(region), sign_region, &len);
    HMAC(EVP_sha256(), sign_region, len, (const unsigned char *)"sls", strlen("sls"), sign_service, &len);
    HMAC(EVP_sha256(), sign_service, len, (const unsigned char *)"aliyun_v4_request", strlen("aliyun_v4_request"), sign_request, &len);
    HMAC(EVP_sha256(), sign_request, len, (const unsigned char *)stringToSign, strlen(stringToSign), sign_key, &len);

    // transfer to hex
    unsigned int i;
    for (i = 0; i < len; i++)
    {
        sprintf(result + (i * 2), "%02x", sign_key[i]);
    }
}

post_log_result * post_logs_from_lz4buf(const char *endpoint, const char * accesskeyId, const char *accessKey, const char *stsToken, const char *project, const char *logstore, lz4_log_buf * buffer, log_post_option * option, auth_version version, const char *region)
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
        md5_to_string((const char *)buffer->data, buffer->length, (char *)md5Buf);


        //puts(md5Buf);
        //puts(nowTime);

        struct curl_slist* headers = NULL;
        
        const char* compress_type_str = "";
        int is_compressed = (option->compress_type != LOG_COMPRESS_NONE);
        switch (option->compress_type)
        {
        case LOG_COMPRESS_LZ4:
            headers = curl_slist_append(headers, "x-log-compresstype:lz4");
            compress_type_str = "x-log-compresstype:lz4\n";
            break;
        case LOG_COMPRESS_NONE:
            break;
        default:
            aos_error_log("unsuported compresstype %d", option->compress_type);
        }

        if (version == AUTH_VERSION_4)
        {
            sds headerRawLen = sdsnewEmpty(64);
            headerRawLen = sdscatprintf(headerRawLen, "x-log-bodyrawsize:%d", (int)buffer->raw_length);
            headers=curl_slist_append(headers, headerRawLen);
            headers=curl_slist_append(headers, "Content-Type:application/x-protobuf");
            sds headerLen= sdsnewEmpty(64);
            headerLen = sdscatprintf(headerLen, "Content-Length:%d", (int)buffer->length);
            headers=curl_slist_append(headers, headerLen);
            headers=curl_slist_append(headers, "x-log-apiversion:0.6.0");

            sds headerHost = sdsnewEmpty(128);
            headerHost = sdscatprintf(headerHost, "Host:%s.%s", project, endpoint);
            headers=curl_slist_append(headers, headerHost);
            
            if (stsToken != NULL)
            {
                sds tokenHeader = sdsnew("x-acs-security-token:");
                tokenHeader = sdscat(tokenHeader, stsToken);
                headers=curl_slist_append(headers, tokenHeader);
                sdsfree(tokenHeader);
            }

            char* contentSha256 = NULL;
            if (buffer->data != NULL)
            {
                contentSha256 = sha256_hash_to_hex((const char *)buffer->data, buffer->length);
            }
            else
            {
                contentSha256 = strdup("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
            }
            sds contentSha256Sds = sdsnewEmpty(128);
            contentSha256Sds = sdscatprintf(contentSha256Sds, "x-log-content-sha256:%s", contentSha256);

            headers=curl_slist_append(headers, contentSha256Sds);

            time_t currentEpochTime = time(NULL);
            struct tm *utcTime = gmtime(&currentEpochTime);
            char currentTime[20];
            char currentDate[9];
            strftime(currentTime, sizeof(currentTime), "%Y%m%dT%H%M%SZ", utcTime);
            strftime(currentDate, sizeof(currentDate), "%Y%m%d", utcTime);

            sds signTime = sdsnewEmpty(64);
            signTime = sdscatprintf(signTime, "x-log-date:%s", currentTime);
            headers=curl_slist_append(headers, signTime);

            sds sigHeaders = sdsnewEmpty(128);
            sds headersToString = sdsnewEmpty(512);
            const char* compress_type_header_str = is_compressed ? "x-log-compresstype;" : "";
            if (stsToken == NULL)
            {
                sigHeaders = sdscatprintf(sigHeaders, "content-type;host;x-log-apiversion;x-log-bodyrawsize;%sx-log-content-sha256;x-log-date", compress_type_header_str);
            
                headersToString = sdscatprintf(headersToString,
                                          "content-type:application/x-protobuf\nhost:%s.%s\nx-log-apiversion:0.6.0\nx-log-bodyrawsize:%d\n%sx-log-content-sha256:%s\nx-log-date:%s\n",
                                           project, endpoint, (int)buffer->raw_length, compress_type_str, contentSha256, currentTime);
            }
            else
            {
                sigHeaders = sdscatprintf(sigHeaders, "content-type;host;x-acs-security-token;x-log-apiversion;x-log-bodyrawsize;%sx-log-content-sha256;x-log-date", compress_type_header_str);
            
                headersToString = sdscatprintf(headersToString,
                                          "content-type:application/x-protobuf\nhost:%s.%s\nx-acs-security-token:%s\nx-log-apiversion:0.6.0\nx-log-bodyrawsize:%d\n%sx-log-content-sha256:%s\nx-log-date:%s\n",
                                           project, endpoint, stsToken, (int)buffer->raw_length, compress_type_str, contentSha256, currentTime);
            }
            sds canonicalRequestStr = sdsnewEmpty(512);
            canonicalRequestStr = sdscatprintf(canonicalRequestStr,
                                              "POST\n/logstores/%s/shards/lb\n\n%s\n%s\n%s", // params is empty
                                              logstore, headersToString, sigHeaders, contentSha256);
            // todo build_canonical_request
            char* canonicalRequestStrSha256 = sha256_hash_to_hex(canonicalRequestStr, strlen(canonicalRequestStr));
            sds scope = sdsnewEmpty(128);
            scope = sdscatprintf(scope, "%s/%s/sls/aliyun_v4_request", currentDate, region);
            sds stringToSign = sdsnewEmpty(512);
            stringToSign = sdscatprintf(stringToSign,
                                        "SLS4-HMAC-SHA256\n%s\n%s\n%s",
                                        currentTime, scope, canonicalRequestStrSha256);
            sds signature = sdsnewEmpty(128);
            buildSignKey(accessKey, region, currentDate, stringToSign, signature);
            // todo build sign key
            sds headerSig = sdsnewEmpty(512);
            headerSig = sdscatprintf(headerSig, "Authorization:SLS4-HMAC-SHA256 Credential=%s/%s,Signature=%s", accesskeyId, scope, signature);
            headers=curl_slist_append(headers, headerSig);

            sdsfree(headerRawLen);
            sdsfree(headerLen);
            sdsfree(headerHost);
            sdsfree(contentSha256Sds);
            sdsfree(signTime);
            sdsfree(sigHeaders);
            sdsfree(headersToString);
            sdsfree(canonicalRequestStr);
            sdsfree(scope);
            sdsfree(stringToSign);
            sdsfree(signature);
            sdsfree(headerSig);
            free(contentSha256);
            free(canonicalRequestStrSha256);
        }
        else
        {
            headers=curl_slist_append(headers, "Content-Type:application/x-protobuf");
            headers=curl_slist_append(headers, "x-log-apiversion:0.6.0");
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
            if (stsToken == NULL)
            {
                sigContent = sdscatprintf(sigContent,
                                          "POST\n%s\napplication/x-protobuf\n%s\nx-log-apiversion:0.6.0\nx-log-bodyrawsize:%d\n%sx-log-signaturemethod:hmac-sha1\n/logstores/%s/shards/lb",
                                          md5Buf, nowTime, (int)buffer->raw_length, compress_type_str, logstore);
            }
            else
            {
                sigContent = sdscatprintf(sigContent,
                                          "POST\n%s\napplication/x-protobuf\n%s\nx-acs-security-token:%s\nx-log-apiversion:0.6.0\nx-log-bodyrawsize:%d\n%sx-log-signaturemethod:hmac-sha1\n/logstores/%s/shards/lb",
                                          md5Buf, nowTime, stsToken, (int)buffer->raw_length, compress_type_str, logstore);
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
            
            sdsfree(headerTime);
            sdsfree(headerMD5);
            sdsfree(headerLen);
            sdsfree(headerRawLen);
            sdsfree(headerHost);
            sdsfree(sigContent);
            sdsfree(headerSig);
        }



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
        /* always cleanup */
        curl_easy_cleanup(curl);
        if (connect_to != NULL)
        {
            curl_slist_free_all(connect_to);
        }
    }


    return result;
}
