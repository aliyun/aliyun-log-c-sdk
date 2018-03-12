#ifndef LIBLOG_API_H
#define LIBLOG_API_H

#include "log_define.h"

#include "log_builder.h"

LOG_CPP_START

/**
 * post log header, with url, path, header items
 */
typedef struct _post_log_header {
    char * url; // post log url, eg `my-log-project.cn-hangzhou.log.aliyuncs.com`
    char * path; // post log path, eg `//logstores/my-logstore/shards/lb"`
    uint8_t item_size; // header size, for pack_logs_from_buffer_lz4 is 10, for pack_logs_from_raw_buffer is 9
    char ** header_items; // header items
}post_log_header;

#ifdef USE_LZ4_FLAG

/**
 * pack log header with lz4 compressed buffer
 * @param endpoint your endpoint, not with 'http' or 'https'
 * @param accesskeyId your access key id
 * @param accessKey your access key secret
 * @param project your project name
 * @param logstore your logstore name
 * @param data you lz4 compressed buffer
 * @param data_size buffer size
 * @param raw_data_size raw buffer size(before compressed)
 * @return post_log_header. if pack fail, post_log_header.header_items will be NULL
 */
post_log_header pack_logs_from_buffer_lz4(const char *endpoint, const char * accesskeyId, const char *accessKey,
                                      const char *project, const char *logstore,
                                      const char * data, uint32_t data_size, uint32_t raw_data_size);
#endif


/**
 *  pack log header with raw buffer
 * @param endpoint your endpoint, not with 'http' or 'https'
 * @param accesskeyId your access key id
 * @param accessKey your access key secret
 * @param project your project name
 * @param logstore your logstore name
 * @param data raw buffer
 * @param raw_data_size raw buffer size
 * @return post_log_header. if pack fail, post_log_header.header_items will be NULL
 */
post_log_header pack_logs_from_raw_buffer(const char *endpoint, const char * accesskeyId, const char *accessKey,
                                      const char *project, const char *logstore,
                                      const char * data, uint32_t raw_data_size);


/**
 * free all buffer holded by this header
 * @param header
 */
void free_post_log_header(post_log_header header);


LOG_CPP_END
#endif
