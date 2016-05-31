#ifndef LIBAOS_UTIL_H
#define LIBAOS_UTIL_H

#include "aos_buf.h"
#include "aos_string.h"
#include "aos_define.h"
#include "aos_fstack.h"

#include "cJSON.h"
#include <apr_md5.h>
#include <apr_sha1.h>

AOS_CPP_START

int aos_parse_json_body(aos_list_t *bc, cJSON **root);

void aos_gnome_sort(const char **headers, int size);

int aos_convert_to_gmt_time(char* date, const char* format, apr_time_exp_t *tm);
int aos_get_gmt_str_time(char datestr[AOS_MAX_GMT_TIME_LEN]);

/**
 * URL-encodes a string from [src] into [dest]. [dest] must have at least
 * 3x the number of characters that [source] has. At most [maxSrcSize] bytes
 * from [src] are encoded; if more are present in [src], 0 is returned from
 * urlEncode, else nonzero is returned.
 */
int aos_url_encode(char *dest, const char *src, int maxSrcSize);

const char* aos_http_method_to_string(http_method_e method);

/**
 * encode query string, check query args < AOS_MAX_QUERY_ARG_LEN
 * result string "?a&b=x"
 */
int aos_query_params_to_string(aos_pool_t *p, aos_table_t *query_params, aos_string_t *querystr);

/**
 * base64 encode bytes. The output buffer must have at least
 * ((4 * (inLen + 1)) / 3) bytes in it.  Returns the number of bytes written
 * to [out].
 */
int aos_base64_encode(const unsigned char *in, int inLen, char *out);

/**
 * Compute HMAC-SHA-1 with key [key] and message [message], storing result
 * in [hmac]
 */
void HMAC_SHA1(unsigned char hmac[20], const unsigned char *key, int key_len,
               const unsigned char *message, int message_len);

unsigned char* aos_md5(aos_pool_t* pool, const char* in, apr_size_t in_len);

int aos_url_decode(const char *in, char *out);

AOS_CPP_END

#endif
