#include "log_util.h"
#include "log_api.h"
#include <string.h>
#include "sds.h"

// set these functions to NULL
md5_to_string_fun g_log_md5_to_string_fun = NULL;
hmac_sha1_signature_to_base64_fun g_log_hmac_sha1_signature_to_base64_fun = NULL;
get_now_time_str_fun g_log_get_now_time_str_fun = NULL;

void free_post_log_header(post_log_header header)
{
    sdsfree(header.url);
    sdsfree(header.path);
    if (header.header_items != NULL)
    {
        uint8_t i = 0;
        for (; i < header.item_size; ++i)
        {
            sdsfree(header.header_items[i]);
        }
        free(header.header_items);
    }
}

post_log_header pack_logs_from_buffer_lz4(const char *endpoint, const char * accesskeyId, const char *accessKey,
                                      const char *project, const char *logstore,
                                      const char * data, uint32_t data_size, uint32_t raw_data_size)
{
    post_log_header header;
    memset(&header, 0, sizeof(post_log_header));
    if (g_log_md5_to_string_fun == NULL || g_log_hmac_sha1_signature_to_base64_fun == NULL || g_log_get_now_time_str_fun == NULL)
    {
        return header;
    }
    // url
    sds url = sdsnew("http://");
    url = sdscat(url, project);
    url = sdscat(url, ".");
    url = sdscat(url, endpoint);

    sds path = sdsnew("/logstores/");
    path = sdscat(path, logstore);
    path = sdscat(path, "/shards/lb");
    header.url = url;
    header.path = path;

    header.item_size = 10;
    header.header_items = (char **)malloc(11 * sizeof(char *));
    memset(header.header_items, 0, 11 * sizeof(char *));

    header.header_items[0] = sdsnew("Content-Type:application/x-protobuf");
    header.header_items[1] = sdsnew("x-log-apiversion:0.6.0");
    header.header_items[2] = sdsnew("x-log-compresstype:lz4");
    header.header_items[3] = sdsnew("x-log-signaturemethod:hmac-sha1");

    char nowTime[64];
    g_log_get_now_time_str_fun(nowTime);

    char md5Buf[33];
    md5Buf[32] = '\0';
    g_log_md5_to_string_fun(data, data_size, (char *)md5Buf);

    sds headerTime = sdsnew("Date:");
    headerTime = sdscat(headerTime, nowTime);
    header.header_items[4] = headerTime;
    sds headerMD5 = sdsnew("Content-MD5:");
    headerMD5 = sdscat(headerMD5, md5Buf);
    header.header_items[5] = headerMD5;

    sds headerLen= sdsnewEmpty(64);
    headerLen = sdscatprintf(headerLen, "Content-Length:%d", data_size);
    header.header_items[6] = headerLen;

    sds headerRawLen = sdsnewEmpty(64);
    headerRawLen = sdscatprintf(headerRawLen, "x-log-bodyrawsize:%d", raw_data_size);
    header.header_items[7] = headerRawLen;

    sds headerHost = sdsnewEmpty(128);
    headerHost = sdscatprintf(headerHost, "Host:%s.%s", project, endpoint);
    header.header_items[8] =  headerHost;


    sds sigContent = sdsnewEmpty(256);
    //sigContent = sdscatprintf(sigContent,
    //                          "POST\n%s\napplication/x-protobuf\n%s\nx-log-apiversion:0.6.0\nx-log-bodyrawsize:%d\nx-log-compresstype:lz4\nx-log-signaturemethod:hmac-sha1\n/logstores/%s/shards/lb",
    //                          md5Buf, nowTime, (int)raw_data_size, logstore);
    sigContent = sdscat(sigContent, "POST\n");
    sigContent = sdscat(sigContent, md5Buf);
    sigContent = sdscat(sigContent, "\napplication/x-protobuf\n");
    sigContent = sdscat(sigContent, nowTime);
    sigContent = sdscat(sigContent, "\nx-log-apiversion:0.6.0\n");
    sigContent = sdscatsds(sigContent, header.header_items[7]);
    sigContent = sdscat(sigContent, "\nx-log-compresstype:lz4\nx-log-signaturemethod:hmac-sha1\n");
    sigContent = sdscatsds(sigContent, path);

    char sha1Buf[128];
    sha1Buf[127] = '\0';
    int destLen = g_log_hmac_sha1_signature_to_base64_fun(sigContent, sdslen(sigContent), accessKey, strlen(accessKey), sha1Buf);
    sdsfree(sigContent);
    sha1Buf[destLen] = '\0';
    sds headerSig = sdsnew("Authorization:LOG ");
    headerSig = sdscat(headerSig, accesskeyId);
    headerSig = sdscat(headerSig, ":");
    headerSig = sdscat(headerSig, sha1Buf);
    header.header_items[9] = headerSig;

    return header;
}


post_log_header pack_logs_from_raw_buffer(const char *endpoint, const char * accesskeyId, const char *accessKey,
                                          const char *project, const char *logstore,
                                          const char * data, uint32_t raw_data_size)
{
    post_log_header header;
    memset(&header, 0, sizeof(post_log_header));
    if (g_log_md5_to_string_fun == NULL || g_log_hmac_sha1_signature_to_base64_fun == NULL || g_log_get_now_time_str_fun == NULL)
    {
        return header;
    }
    // url
    sds url = sdsnew("http://");
    url = sdscat(url, project);
    url = sdscat(url, ".");
    url = sdscat(url, endpoint);

    sds path = sdsnew("/logstores/");
    path = sdscat(path, logstore);
    path = sdscat(path, "/shards/lb");
    header.url = url;
    header.path = path;

    header.item_size = 9;
    header.header_items = (char **)malloc(10 * sizeof(char *));
    memset(header.header_items, 0, 10 * sizeof(char *));

    header.header_items[0] = sdsnew("Content-Type:application/x-protobuf");
    header.header_items[1] = sdsnew("x-log-apiversion:0.6.0");
    header.header_items[2] = sdsnew("x-log-signaturemethod:hmac-sha1");

    char nowTime[64];
    g_log_get_now_time_str_fun(nowTime);

    char md5Buf[33];
    md5Buf[32] = '\0';
    g_log_md5_to_string_fun(data, raw_data_size, (char *)md5Buf);

    sds headerTime = sdsnew("Date:");
    headerTime = sdscat(headerTime, nowTime);
    header.header_items[3] = headerTime;
    sds headerMD5 = sdsnew("Content-MD5:");
    headerMD5 = sdscat(headerMD5, md5Buf);
    header.header_items[4] = headerMD5;

    sds headerLen= sdsnewEmpty(64);
    headerLen = sdscatprintf(headerLen, "Content-Length:%d", raw_data_size);
    header.header_items[5] = headerLen;

    sds headerRawLen = sdsnewEmpty(64);
    headerRawLen = sdscatprintf(headerRawLen, "x-log-bodyrawsize:%d", raw_data_size);
    header.header_items[6] = headerRawLen;

    sds headerHost = sdsnewEmpty(128);
    headerHost = sdscatprintf(headerHost, "Host:%s.%s", project, endpoint);
    header.header_items[7] =  headerHost;


    sds sigContent = sdsnewEmpty(256);
    sigContent = sdscat(sigContent, "POST\n");
    sigContent = sdscat(sigContent, md5Buf);
    sigContent = sdscat(sigContent, "\napplication/x-protobuf\n");
    sigContent = sdscat(sigContent, nowTime);
    sigContent = sdscat(sigContent, "\nx-log-apiversion:0.6.0\n");
    sigContent = sdscatsds(sigContent, header.header_items[6]);
    sigContent = sdscat(sigContent, "\nx-log-signaturemethod:hmac-sha1\n");
    sigContent = sdscatsds(sigContent, path);

    char sha1Buf[128];
    sha1Buf[127] = '\0';
    int destLen = g_log_hmac_sha1_signature_to_base64_fun(sigContent, sdslen(sigContent), accessKey, strlen(accessKey), sha1Buf);
    sdsfree(sigContent);
    sha1Buf[destLen] = '\0';
    sds headerSig = sdsnew("Authorization:LOG ");
    headerSig = sdscat(headerSig, accesskeyId);
    headerSig = sdscat(headerSig, ":");
    headerSig = sdscat(headerSig, sha1Buf);
    header.header_items[8] = headerSig;

    return header;
}


