#include "log_builder.h"
#include "lz4.h"
#include "sds.h"
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "inner_log.h"

// 1+3( 1 --->  header;  2 ---> 128 * 128 = 16KB)
#define INIT_LOG_SIZE_BYTES 3

/**
 * Return the number of bytes required to store a variable-length unsigned
 * 32-bit integer in base-128 varint encoding.
 *
 * \param v
 *      Value to encode.
 * \return
 *      Number of bytes required.
 */
static inline size_t uint32_size(uint32_t v)
{
    if (v < (1UL << 7)) {
        return 1;
    } else if (v < (1UL << 14)) {
        return 2;
    } else if (v < (1UL << 21)) {
        return 3;
    } else if (v < (1UL << 28)) {
        return 4;
    } else {
        return 5;
    }
}

/**
 * Pack an unsigned 32-bit integer in base-128 varint encoding and return the
 * number of bytes written, which must be 5 or less.
 *
 * \param value
 *      Value to encode.
 * \param[out] out
 *      Packed value.
 * \return
 *      Number of bytes written to `out`.
 */
static inline size_t uint32_pack(uint32_t value, uint8_t *out)
{
    unsigned rv = 0;

    if (value >= 0x80) {
        out[rv++] = value | 0x80;
        value >>= 7;
        if (value >= 0x80) {
            out[rv++] = value | 0x80;
            value >>= 7;
            if (value >= 0x80) {
                out[rv++] = value | 0x80;
                value >>= 7;
                if (value >= 0x80) {
                    out[rv++] = value | 0x80;
                    value >>= 7;
                }
            }
        }
    }
    /* assert: value<128 */
    out[rv++] = value;
    return rv;
}

static inline uint32_t parse_uint32(unsigned len, const uint8_t *data)
{
    uint32_t rv = data[0] & 0x7f;
    if (len > 1) {
        rv |= ((uint32_t) (data[1] & 0x7f) << 7);
        if (len > 2) {
            rv |= ((uint32_t) (data[2] & 0x7f) << 14);
            if (len > 3) {
                rv |= ((uint32_t) (data[3] & 0x7f) << 21);
                if (len > 4)
                    rv |= ((uint32_t) (data[4]) << 28);
            }
        }
    }
    return rv;
}

static unsigned scan_varint(unsigned len, const uint8_t *data)
{
    unsigned i;
    if (len > 10)
        len = 10;
    for (i = 0; i < len; i++)
        if ((data[i] & 0x80) == 0)
            break;
    if (i == len)
        return 0;
    return i + 1;
}

log_group_builder* log_group_create()
{
    log_group_builder* bder = (log_group_builder*)malloc(sizeof(log_group_builder)+sizeof(log_group));
    memset(bder, 0, sizeof(log_group_builder)+sizeof(log_group));
    bder->grp = (log_group*)((char *)(bder) + sizeof(log_group_builder));
    bder->loggroup_size = sizeof(log_group) + sizeof(log_group_builder);
    bder->builder_time = time(NULL);
    bder->start_uuid = -1;
    bder->end_uuid = -1;
    return bder;
}

void log_group_destroy(log_group_builder* bder)
{
    // free tag
    log_group* group = bder->grp;
    if (group->tags.buffer != NULL)
    {
        free(group->tags.buffer);
    }
    // free log
    if (group->logs.buffer != NULL)
    {
        free(group->logs.buffer);
    }
    if (group->topic != NULL)
    {
        sdsfree(group->topic);
    }
    if (group->source != NULL)
    {
        sdsfree(group->source);
    }
    // free self
    free(bder);
}


/**
 * adjust buffer, this function will ensure tag's buffer size >= tag->now_buffer_len + new_len
 * @param tag
 * @param new_len new buffer len
 */
void _adjust_buffer(log_tag * tag, uint32_t new_len)
{
    if (tag->buffer == NULL)
    {
        tag->buffer = (char *)malloc(new_len << 2);
        tag->max_buffer_len = new_len << 2;
        tag->now_buffer = tag->buffer;
        tag->now_buffer_len = 0;
        return;
    }
    uint32_t new_buffer_len = tag->max_buffer_len << 1;

    if (new_buffer_len < tag->now_buffer_len + new_len)
    {
        new_buffer_len = tag->now_buffer_len + new_len;
    }

    tag->buffer = (char *)realloc(tag->buffer, new_buffer_len);
    tag->now_buffer = tag->buffer + tag->now_buffer_len;
    tag->max_buffer_len = new_buffer_len;
}

void add_log_raw(log_group_builder *bder, const char *buffer, size_t size)
{
    ++bder->grp->n_logs;
    log_tag * log = &(bder->grp->logs);
    if (log->now_buffer == NULL || log->max_buffer_len < log->now_buffer_len + size)
    {
        _adjust_buffer(log, size);
    }
    memcpy(log->now_buffer, buffer, size);
    bder->loggroup_size += size;
    log->now_buffer_len += size;
    log->now_buffer += size;
}

void add_log_full(log_group_builder* bder, uint32_t logTime, int32_t pair_count, char ** keys, size_t * key_lens, char ** values, size_t * val_lens)
{
    ++bder->grp->n_logs;

    // limit logTime's min value, ensure varint size is 5
    if (logTime < 1263563523)
    {
        logTime = 1263563523;
    }

    int32_t i = 0;
    int32_t logSize = 6;
    for (; i < pair_count; ++i)
    {
        uint32_t contSize = uint32_size(key_lens[i]) + uint32_size(val_lens[i]) + key_lens[i] + val_lens[i] + 2;
        logSize += 1 + uint32_size(contSize) + contSize;
    }
    int32_t totalBufferSize = logSize + 1 + uint32_size(logSize);

    log_tag * log = &(bder->grp->logs);

    if (log->now_buffer == NULL || log->max_buffer_len < log->now_buffer_len + totalBufferSize)
    {
        _adjust_buffer(log, totalBufferSize);
    }


    bder->loggroup_size += totalBufferSize;
    uint8_t * buf = (uint8_t*)log->now_buffer;

    *buf++ = 0x0A;
    buf += uint32_pack(logSize, buf);

    // time
    *buf++=0x08;
    buf += uint32_pack(logTime, buf);

    // Content
    // header
    i = 0;
    for (; i < pair_count; ++i)
    {
        *buf++ = 0x12;
        buf += uint32_pack(uint32_size(key_lens[i]) + uint32_size(val_lens[i]) + 2 + key_lens[i] + val_lens[i], buf);
        *buf++ = 0x0A;
        buf += uint32_pack(key_lens[i], buf);
        memcpy(buf, keys[i], key_lens[i]);
        buf += key_lens[i];
        *buf++ = 0x12;
        buf += uint32_pack(val_lens[i], buf);
        memcpy(buf, values[i], val_lens[i]);
        buf += val_lens[i];
    }
    assert(buf - (uint8_t*)log->now_buffer == totalBufferSize);
    log->now_buffer_len += totalBufferSize;
    log->now_buffer = (char *)buf;
}


void add_source(log_group_builder* bder,const char* src,size_t len)
{
    bder->loggroup_size += sizeof(char)*(len) + uint32_size((uint32_t)len) + 1;
    bder->grp->source = sdsnewlen(src, len);
}

void add_topic(log_group_builder* bder,const char* tpc,size_t len)
{
    bder->loggroup_size += sizeof(char)*(len) + uint32_size((uint32_t)len) + 1;
    bder->grp->topic = sdsnewlen(tpc, len);
}

void add_pack_id(log_group_builder* bder, const char* pack, size_t pack_len, size_t packNum)
{
    char packStr[128];
    packStr[127] = '\0';
    snprintf(packStr, 127, "%s-%X", pack, (unsigned int)packNum);
    add_tag(bder, "__pack_id__", strlen("__pack_id__"), packStr, strlen(packStr));
}


void add_tag(log_group_builder* bder, const char* k, size_t k_len, const char* v, size_t v_len)
{
    // use only 1 malloc
    uint32_t tag_size = sizeof(char) * (k_len + v_len) + uint32_size((uint32_t)k_len) + uint32_size((uint32_t)v_len) + 2;
    uint32_t n_buffer = 1 + uint32_size(tag_size) + tag_size;
    log_tag * tag = &(bder->grp->tags);
    if (tag->now_buffer == NULL || tag->now_buffer_len + n_buffer > tag->max_buffer_len)
    {
        _adjust_buffer(tag, n_buffer);
    }
    uint8_t * buf = (uint8_t *)tag->now_buffer;
    *buf++ = 0x32;
    buf += uint32_pack(tag_size, buf);
    *buf++ = 0x0A;
    buf += uint32_pack((uint32_t)k_len, buf);
    memcpy(buf, k, k_len);
    buf += k_len;
    *buf++ = 0x12;
    buf += uint32_pack((uint32_t)v_len, buf);
    memcpy(buf, v, v_len);
    buf += v_len;
    assert((uint8_t *)tag->now_buffer + n_buffer == buf);
    tag->now_buffer = (char *)buf;
    tag->now_buffer_len += n_buffer;
    bder->loggroup_size += n_buffer;
}

static uint32_t _log_pack(log_group * grp, uint8_t * buf)
{
    uint8_t * start_buf = buf;

    if (grp->logs.buffer != NULL)
    {
        buf += grp->logs.now_buffer_len;
    }
    else
    {
        return 0;
    }

    if (grp->topic != NULL)
    {
        *buf++ = 0x1A;
        buf+= uint32_pack((uint32_t)sdslen(grp->topic), buf);
        memcpy(buf, grp->topic, sdslen(grp->topic));
        buf += sdslen(grp->topic);
    }

    if (grp->source != NULL)
    {
        *buf++ = 0x22;
        buf+= uint32_pack((uint32_t)sdslen(grp->source), buf);
        memcpy(buf, grp->source, sdslen(grp->source));
        buf += sdslen(grp->source);
    }

    if (grp->tags.buffer != NULL)
    {
        memcpy(buf, grp->tags.buffer, grp->tags.now_buffer_len);
        buf += grp->tags.now_buffer_len;
    }

    return buf - start_buf;
}

void fix_log_group_time(char * pb_buffer, size_t len, uint32_t new_time)
{
    if (len == 0 || pb_buffer == NULL || new_time < 1263563523)
    {
        return;
    }
    if (pb_buffer[0] != 0x0A)
    {
        return;
    }
    uint8_t * buf = (uint8_t *)pb_buffer;
    uint8_t * startBuf = (uint8_t *)pb_buffer;
    while (buf - startBuf < len && *buf == 0x0A)
    {
        ++buf;
        unsigned logSizeLen = scan_varint(5, buf);
        uint32_t logSize = parse_uint32(logSizeLen, buf);
        buf += logSizeLen;
        // time
        if (*buf == 0x08)
        {
            uint32_pack(new_time, buf + 1);
        }
        buf += logSize;
    }

}

sds put_val(sds s, char* val)
{
    s = sdscat(s, "\"");
    s = sdscat(s, val);
    s = sdscat(s, "\"");
    return s;
}

sds put_kv_with_comma(sds s, char *key, char *val, int comma)
{
    s = put_val(s, key);
    s = sdscat(s, ":");
    s = put_val(s, val);
    if (comma)
    {
        s = sdscat(s, ",");
    }
    return s;
}


sds escape_json(char **value) {
    size_t len = strlen(*value);
    sds result = sdsnewEmpty(len);
    for (int i = 0; i < len; i ++) {
        switch ((*value)[i]) {
            case '"': result = sdscat(result, "\\\""); break;
            case '\\': result = sdscat(result, "\\\\"); break;
            case '\b': result = sdscat(result, "\\b"); break;
            case '\f': result = sdscat(result, "\\f"); break;
            case '\n': result = sdscat(result, "\\n"); break;
            case '\r': result = sdscat(result, "\\r"); break;
            case '\t': result = sdscat(result, "\\t"); break;
            default:
                if ('\x00' <= (*value)[i] && (*value)[i] <= '\x1f') {
                    result = sdscatprintf(result, "%s%04X","\\u", (int)(*value)[i]);
                } else {
                    result = sdscatchar(result, (*value)[i]);
                }
        }
    }
    return result;
}

sds put_kv(sds s, char *key, char *val)
{
    sds v = escape_json(&val);
    s =  put_kv_with_comma(s, key, v, 1);
    sdsfree(v);

    return s;
}

sds put_kv_no_comma(sds s, char *key, char *val)
{
    return put_kv_with_comma(s, key, val, 0);
}

sds put_array(sds s, char *key, sds array)
{
    s = put_val(s, key);
    s = sdscat(s, ":");
    s = sdscat(s, array);
    return s;
}

sds remove_comma(sds s) {
    sds ns = sdsnewlen(s, sdslen(s) - 1);
    sdsfree(s);
    return ns;
}

unsigned int read_length_from_pb(const uint8_t *data)
{
    return scan_varint(5, data);
}

uint32_t read_chars_from_pb(uint8_t **data, char **chars)
{
    (*data) ++;
    unsigned int len = read_length_from_pb(*data);
    uint32_t _len = parse_uint32(len, *data);
    (*data) += len;

    *chars = (char *) malloc(sizeof (char) * (_len + 1) );
    memset(*chars, 0, _len + 1);
    memcpy(*chars, *data, _len + 1);
    (*chars)[_len] = '\0';

    (*data) += _len;

    return _len;
}

extern size_t serialize_pb_buffer_to_webtracking(char *pb_buffer, size_t len, char **new_buffer)
{
    if (0 == len || NULL == pb_buffer)
    {
        return 0;
    }

    if (0x0A != pb_buffer[0])
    {
        return 0;
    }

    sds root = sdsnew("{");
    sds _root_logs_ = sdsnew("[");
    sds _root_tags_ = sdsnew("{");

    uint8_t * buf = (uint8_t *)pb_buffer;
    uint8_t * startBuf = (uint8_t *)pb_buffer;
    // log package
    while (buf - startBuf < len && *buf == 0x0A)
    {
        aos_info_log("serialize_pb_buffer_to_webtracking, start process single log.");

        ++buf;
        unsigned logSizeLen = read_length_from_pb(buf);
        buf += logSizeLen;

        uint32_t time = 0;
        // time
        if (*buf == 0x08)
        {
            buf++;
            unsigned timeLen = read_length_from_pb(buf);
            if (timeLen != 5)
            {
                return 0;
            }
            time = parse_uint32(timeLen, buf);
            buf += timeLen;

            aos_info_log("serialize_pb_buffer_to_webtracking, time: %d", time);
        }

        sds _log_ = sdsnew("{");
        if (time)
        {
            _log_ = sdscatprintf(_log_, "\"__time__\":%u,", time);
        }

        // Content
        // Header
        while (*buf == 0x12)
        {
            buf++;
            unsigned kvLen = read_length_from_pb(buf);
            buf += kvLen;

            char *key = NULL;
            char *val = NULL;
            // key
            if (*buf == 0x0A)
            {
                read_chars_from_pb(&buf, &key);
            }

            // value
            if (*buf == 0x12)
            {
                read_chars_from_pb(&buf, &val);
            }

            if (key && val)
            {
                _log_ = put_kv(_log_, key, val);
            }

            aos_info_log("serialize_pb_buffer_to_webtracking, content {%s: %s}", key, val);

            free(key);
            free(val);
        }

        if (sdslen(_log_) > 1)
        {
            // remove last ','
            _log_ = remove_comma(_log_);
        }

        _log_ = sdscat(_log_, "}");
        _root_logs_ = sdscat(_root_logs_, _log_);
        _root_logs_ = sdscat(_root_logs_, ",");
        sdsfree(_log_);

        // Topic
        if (0x1A == *buf)
        {
            char *topic;
            read_chars_from_pb(&buf, &topic);
            aos_info_log("serialize_pb_buffer_to_webtracking, topic: %s", topic);
            root = put_kv(root, "__topic__", topic);
            free(topic);
        }

        // Source
        if (0x22 == *buf)
        {
            char *source;
            read_chars_from_pb(&buf, &source);
            aos_info_log("serialize_pb_buffer_to_webtracking, source: %s", source);

            root = put_kv(root, "__source__", source);
            free(source);
        }

        // Tag
        while (0x32 == *buf)
        {
            buf ++;

            unsigned int tagLen = read_length_from_pb(buf);
            buf += tagLen;

            char *key = NULL;
            char *val = NULL;
            // key
            if (*buf == 0x0A)
            {
                read_chars_from_pb(&buf, &key);
            }

            // value
            if (*buf == 0x12)
            {
                read_chars_from_pb(&buf, &val);
            }

            if (key && val)
            {
                _root_tags_ = put_kv(_root_tags_, key, val);
            }

            aos_info_log("serialize_pb_buffer_to_webtracking, tag {%s: %s}", key, val);

            free(key);
            free(val);
        }
    }

    aos_info_log("serialize_pb_buffer_to_webtracking, log package has been processed.");

    if (sdslen(_root_logs_) > 1)
    {
        _root_logs_ = remove_comma(_root_logs_);
    }
    _root_logs_ = sdscat(_root_logs_, "]");
    _root_logs_ = sdscat(_root_logs_, ",");
    root = put_array(root, "__logs__", _root_logs_);

    if (sdslen(_root_tags_) > 1)
    {
        _root_tags_ = remove_comma(_root_tags_);
    }
    _root_tags_ = sdscat(_root_tags_, "}");
    root = put_array(root, "__tags__", _root_tags_);
    root = sdscat(root, "}");

    size_t root_len = sdslen(root);
    *new_buffer = (char *)malloc(sizeof(char) * root_len);
    memcpy(*new_buffer, root, root_len);

    sdsfree(_root_logs_);
    sdsfree(_root_tags_);
    sdsfree(root);

    aos_info_log("serialize_pb_buffer_to_webtracking, json: %s", *new_buffer);
    return root_len;
}

void fix_log_time(char * pb_buffer, size_t len, uint32_t new_time)
{
    if (len == 0 || pb_buffer == NULL || new_time < 1263563523)
    {
        return;
    }
    if (pb_buffer[0] != 0x0A)
    {
        return;
    }
    ++pb_buffer;
    uint8_t * buf = (uint8_t *)pb_buffer;
    unsigned logSizeLen = scan_varint(5, buf);
    buf += logSizeLen;
    // time
    if (*buf == 0x08)
    {
        unsigned timeLen = scan_varint(5, buf + 1);
        if (timeLen != 5)
        {
            return;
        }
        uint32_pack(new_time, buf + 1);
    }
}

uint32_t get_log_time(const char * pb_buffer, size_t len)
{
    if (len == 0 || pb_buffer == NULL)
    {
        return time(NULL);
    }
    if (pb_buffer[0] != 0x0A)
    {
        return time(NULL);
    }
    ++pb_buffer;
    uint8_t * buf = (uint8_t *)pb_buffer;
    unsigned logSizeLen = scan_varint(5, buf);
    buf += logSizeLen;
    // time
    if (*buf == 0x08)
    {
        unsigned timeLen = scan_varint(5, buf + 1);
        return parse_uint32(timeLen, buf + 1);
    }
    return time(NULL);
}


log_buf serialize_to_proto_buf_with_malloc(log_group_builder* bder)
{
    log_buf buf;
    buf.buffer = NULL;
    buf.n_buffer = 0;
    log_tag * log = &(bder->grp->logs);
    if (log->buffer == NULL)
    {
        return buf;
    }
    if (log->max_buffer_len < bder->loggroup_size)
    {
        _adjust_buffer(log, bder->loggroup_size - log->now_buffer_len);
    }
    buf.n_buffer = _log_pack(bder->grp, (uint8_t *)log->buffer);
    buf.buffer = log->buffer;
    return buf;
}

lz4_log_buf* serialize_to_proto_buf_with_malloc_no_lz4(log_group_builder* bder)
{
  log_buf buf = serialize_to_proto_buf_with_malloc(bder);
  lz4_log_buf* pLogbuf = (lz4_log_buf*)malloc(sizeof(lz4_log_buf) + buf.n_buffer);
  pLogbuf->length = buf.n_buffer;
  pLogbuf->raw_length = buf.n_buffer;
  memcpy(pLogbuf->data, buf.buffer, buf.n_buffer);
  return pLogbuf;
}

lz4_log_buf* serialize_to_proto_buf_with_malloc_lz4(log_group_builder* bder)
{
    log_tag * log = &(bder->grp->logs);
    if (log->buffer == NULL)
    {
        return NULL;
    }
    if (log->max_buffer_len < bder->loggroup_size)
    {
        _adjust_buffer(log, bder->loggroup_size - log->now_buffer_len);
    }
    size_t length = _log_pack(bder->grp, (uint8_t *)log->buffer);
    // @debug
    //FILE * pFile = fopen("dump.dat", "wb+");
    //fwrite(log->buffer, 1, length, pFile);
    //fclose(pFile);
    // @debug end
    int compress_bound = LZ4_compressBound(length);
    char *compress_data = (char *)malloc(compress_bound);
    int compressed_size = LZ4_compress_default((char *)log->buffer, compress_data, length, compress_bound);
    if(compressed_size <= 0)
    {
        free(compress_data);
        return NULL;
    }
    lz4_log_buf* pLogbuf = (lz4_log_buf*)malloc(sizeof(lz4_log_buf) + compressed_size);
    pLogbuf->length = compressed_size;
    pLogbuf->raw_length = length;
    memcpy(pLogbuf->data, compress_data, compressed_size);
    free(compress_data);
    return pLogbuf;
}

void free_lz4_log_buf(lz4_log_buf* pBuf)
{
    free(pBuf);
}

#ifdef LOG_KEY_VALUE_FLAG

void add_log_begin(log_group_builder * bder)
{
    log_tag * logs = &bder->grp->logs;
    if (logs->buffer == NULL || logs->now_buffer_len + INIT_LOG_SIZE_BYTES > logs->max_buffer_len)
    {
        _adjust_buffer(logs, INIT_LOG_SIZE_BYTES);
    }
    bder->grp->log_now_buffer = logs->now_buffer + INIT_LOG_SIZE_BYTES;
}

void add_log_time(log_group_builder * bder, uint32_t logTime)
{
    log_tag * logs = &bder->grp->logs;
    // 1 header and 5 time
    if (bder->grp->log_now_buffer - logs->buffer + 6 > logs->max_buffer_len)
    {
        // reset log_now_buffer
        size_t delta = bder->grp->log_now_buffer - logs->buffer;
        _adjust_buffer(logs, delta + 6);
        bder->grp->log_now_buffer = logs->buffer + delta;
    }

    uint8_t * buf = (uint8_t *)bder->grp->log_now_buffer;
    // time
    *buf++=0x08;
    buf += uint32_pack(logTime, buf);
    bder->grp->log_now_buffer = (char *)buf;
}

void add_log_key_value(log_group_builder *bder, const char * key, size_t key_len, const char * value, size_t value_len)
{
    // sum total size
    uint32_t kv_size = sizeof(char) * (key_len + value_len) + uint32_size((uint32_t)key_len) + uint32_size((uint32_t)value_len) + 2;
    kv_size += 1 + uint32_size(kv_size);
    log_tag * logs = &bder->grp->logs;
    // ensure buffer size
    if (bder->grp->log_now_buffer - logs->buffer + kv_size > logs->max_buffer_len )
    {
        // reset log_now_buffer
        size_t delta = bder->grp->log_now_buffer - logs->buffer;
        _adjust_buffer(logs, delta + kv_size);
        bder->grp->log_now_buffer = logs->buffer + delta;
    }
    uint8_t * buf = (uint8_t *)bder->grp->log_now_buffer;
    // key_value header
    *buf++ = 0x12;
    buf += uint32_pack(uint32_size(key_len) + uint32_size(value_len) + 2 + key_len + value_len, buf);
    // key len
    *buf++ = 0x0A;
    buf += uint32_pack(key_len, buf);
    // key
    memcpy(buf, key, key_len);
    buf += key_len;
    // value len
    *buf++ = 0x12;
    buf += uint32_pack(value_len, buf);
    // value
    memcpy(buf, value, value_len);
    buf += value_len;
    bder->grp->log_now_buffer = (char *)buf;
}

void add_log_end(log_group_builder * bder)
{
    log_tag * logs = &bder->grp->logs;
    uint32_t log_size = bder->grp->log_now_buffer - logs->now_buffer - INIT_LOG_SIZE_BYTES;
    // check total size and uint32_size(total size)

    int32_t header_size = uint32_size(log_size) + 1;

    if (header_size != INIT_LOG_SIZE_BYTES)
    {
        int32_t delta_header_size = header_size - (int32_t)INIT_LOG_SIZE_BYTES;
        // need check buffer size
        if (delta_header_size > 0 && bder->grp->log_now_buffer - logs->buffer +  delta_header_size > logs->max_buffer_len)
        {
            // reset log_now_buffer
            size_t delta = bder->grp->log_now_buffer - logs->buffer;
            _adjust_buffer(logs, delta + delta_header_size);
            bder->grp->log_now_buffer = logs->buffer + delta;
        }
        // move buffer
        memmove(logs->now_buffer + header_size, logs->now_buffer + INIT_LOG_SIZE_BYTES, log_size);
    }
    // set log header
    uint8_t * buf = (uint8_t *)logs->now_buffer;
    *buf++ = 0x0A;
    buf += uint32_pack(log_size, buf);
    // do not need to add header_size
    logs->now_buffer = (char *)buf + log_size;
    logs->now_buffer_len += header_size + log_size;
    // update loggroup size
    bder->loggroup_size += header_size + log_size;
}

void clear_log_tag(log_tag *tag)
{
    tag->now_buffer = tag->buffer;
    tag->now_buffer_len = 0;
}

void
add_log_full_v2(log_group_builder *bder, uint32_t logTime, size_t logItemCount,
                const char *logItemsBuf, const uint32_t *logItemsSize)
{
    if (logTime < 1263563523)
    {
        logTime = time(NULL);
    }
    add_log_begin(bder);
    add_log_time(bder, logTime);
    size_t startOffset = 0;
    logItemCount = (logItemCount >> 1) << 1;
    for (size_t (i) = 0; (i) < logItemCount; i += 2)
    {
        uint32_t keySize = logItemsSize[i];
        uint32_t valSize = logItemsSize[i+1];
        add_log_key_value(bder, logItemsBuf + startOffset, keySize, logItemsBuf + startOffset + keySize, valSize);
        startOffset += keySize + valSize;
    }
    add_log_end(bder);
}

void add_log_full_int32(log_group_builder *bder, uint32_t logTime,
                        int32_t pair_count, char **keys, int32_t *key_lens,
                        char **values, int32_t *val_lens)
{
    ++bder->grp->n_logs;

    // limit logTime's min value, ensure varint size is 5
    if (logTime < 1263563523)
    {
        logTime = 1263563523;
    }

    int32_t i = 0;
    int32_t logSize = 6;
    for (; i < pair_count; ++i)
    {
        uint32_t contSize = uint32_size(key_lens[i]) + uint32_size(val_lens[i]) + key_lens[i] + val_lens[i] + 2;
        logSize += 1 + uint32_size(contSize) + contSize;
    }
    int32_t totalBufferSize = logSize + 1 + uint32_size(logSize);

    log_tag * log = &(bder->grp->logs);

    if (log->now_buffer == NULL || log->max_buffer_len < log->now_buffer_len + totalBufferSize)
    {
        _adjust_buffer(log, totalBufferSize);
    }


    bder->loggroup_size += totalBufferSize;
    uint8_t * buf = (uint8_t*)log->now_buffer;

    *buf++ = 0x0A;
    buf += uint32_pack(logSize, buf);

    // time
    *buf++=0x08;
    buf += uint32_pack(logTime, buf);

    // Content
    // header
    i = 0;
    for (; i < pair_count; ++i)
    {
        *buf++ = 0x12;
        buf += uint32_pack(uint32_size(key_lens[i]) + uint32_size(val_lens[i]) + 2 + key_lens[i] + val_lens[i], buf);
        *buf++ = 0x0A;
        buf += uint32_pack(key_lens[i], buf);
        memcpy(buf, keys[i], key_lens[i]);
        buf += key_lens[i];
        *buf++ = 0x12;
        buf += uint32_pack(val_lens[i], buf);
        memcpy(buf, values[i], val_lens[i]);
        buf += val_lens[i];
    }
    assert(buf - (uint8_t*)log->now_buffer == totalBufferSize);
    log->now_buffer_len += totalBufferSize;
    log->now_buffer = (char *)buf;
}

#endif

