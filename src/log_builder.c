#include "log_builder.h"
#include "lz4.h"
#include "sds.h"
#include <string.h>
#include <stdio.h>
#include <assert.h>

#define INIT_TAG_NUMBER_IN_LOGGROUP 8
#define INIT_LOG_NUMBER_IN_LOGGROUP 8

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

void add_log_full(log_group_builder* bder, uint32_t logTime, int32_t pair_count, char ** keys, size_t * key_lens, char ** values, size_t * val_lens)
{
    ++bder->grp->n_logs;
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

