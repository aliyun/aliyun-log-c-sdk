#include "aos_buf.h"
#include "aos_log.h"
#include <apr_file_io.h>

aos_buf_t *aos_create_buf(aos_pool_t *p, int size)
{
    aos_buf_t* b;

    b = aos_palloc(p, sizeof(aos_buf_t) + size);
    if (b == NULL) {
        return NULL;
    }

    b->pos = (uint8_t *)b + sizeof(aos_buf_t);
    b->start = b->pos;
    b->last = b->start;
    b->end = b->last + size;
    aos_list_init(&b->node);

    return b;
}

aos_buf_t *aos_buf_pack(aos_pool_t *p, const void *data, int size)
{
    aos_buf_t* b;

    b = aos_palloc(p, sizeof(aos_buf_t));
    if (b == NULL) {
        return NULL;
    }

    b->pos = (uint8_t *)data;
    b->start = b->pos;
    b->last = b->start + size;
    b->end = b->last;
    aos_list_init(&b->node);

    return b;
}

int64_t aos_buf_list_len(aos_list_t *list)
{
    aos_buf_t *b;
    int64_t len = 0;

    aos_list_for_each_entry(b, list, node) {
        len += aos_buf_size(b);
    }

    return len;
}

char *aos_buf_list_content(aos_pool_t *p, aos_list_t *list)
{
    int64_t body_len;
    char *buf;
    int64_t pos = 0;
    int64_t size = 0;
    aos_buf_t *content;

    body_len = aos_buf_list_len(list);
    buf = aos_pcalloc(p, (size_t)(body_len + 1));
    buf[body_len] = '\0';
    aos_list_for_each_entry(content, list, node) {
        size = aos_buf_size(content);
        memcpy(buf + pos, content->pos, (size_t)(size));
        pos += size;
    }
    return buf;
}

aos_file_buf_t *aos_create_file_buf(aos_pool_t *p)
{
    return (aos_file_buf_t*)aos_pcalloc(p, sizeof(aos_file_buf_t));
}

int aos_open_file_for_read(aos_pool_t *p, const char *path, aos_file_buf_t *fb)
{
    int s;
    char buf[256];
    apr_finfo_t finfo;

    if ((s = apr_file_open(&fb->file, path, APR_READ, APR_UREAD | APR_GREAD, p)) != APR_SUCCESS) {
        aos_error_log("apr_file_open failure, code:%d %s.", s, apr_strerror(s, buf, sizeof(buf)));
        assert(fb->file == NULL);
        return AOSE_OPEN_FILE_ERROR;
    }

    if ((s = apr_file_info_get(&finfo, APR_FINFO_NORM, fb->file)) != APR_SUCCESS) {
        apr_file_close(fb->file);
        aos_error_log("apr_file_open failure, code:%d %s.", s, apr_strerror(s, buf, sizeof(buf)));
        return AOSE_FILE_INFO_ERROR;
    }
    fb->file_pos = 0;
    fb->file_last = finfo.size;
    fb->owner = 1;

    return AOSE_OK;
}

int aos_open_file_for_all_read(aos_pool_t *p, const char *path, aos_file_buf_t *fb)
{
    return aos_open_file_for_read(p, path, fb);
}

int aos_open_file_for_range_read(aos_pool_t *p, const char *path, 
    int64_t file_pos, int64_t file_last, aos_file_buf_t *fb)
{
    int s;

    s = aos_open_file_for_read(p, path, fb);
    if (s == AOSE_OK) {
        if (file_pos > fb->file_pos) {
            if (file_pos > fb->file_last) {
                aos_warn_log("read range beyond file size, read start:%" APR_INT64_T_FMT ", file size:%" APR_INT64_T_FMT "\n", 
                    file_pos, fb->file_last);
                file_pos = fb->file_last;
            }
            fb->file_pos = file_pos;
        }
        if (file_last < fb->file_last) {
            fb->file_last = file_last;
        }
        apr_file_seek(fb->file, APR_SET, (apr_off_t *)&fb->file_pos);
    }

    return s;
}

int aos_open_file_for_write(aos_pool_t *p, const char *path, aos_file_buf_t *fb)
{
    int s;
    char buf[256];

    if ((s = apr_file_open(&fb->file, path, APR_CREATE | APR_WRITE | APR_TRUNCATE,
                APR_UREAD | APR_UWRITE | APR_GREAD, p)) != APR_SUCCESS) {
        aos_error_log("apr_file_open failure, code:%d %s.", s, apr_strerror(s, buf, sizeof(buf)));
        assert(fb->file == NULL);
        return AOSE_OPEN_FILE_ERROR;
    }
    fb->owner = 1;

    return AOSE_OK;
}

void aos_buf_append_string(aos_pool_t *p, aos_buf_t *b, const char *str, int len)
{
    int size;
    int nsize;
    int remain;
    char *buf;

    if (len <= 0) return;

    remain = b->end - b->last;

    if (remain > len + 128) {
        memcpy(b->last, str, len);
        b->last += len;
    } else {
        size = aos_buf_size(b);
        nsize = (size + len) * 2;
        buf = aos_palloc(p, nsize);
        memcpy(buf, b->pos, size);
        memcpy(buf+size, str, len);
        b->start = (uint8_t *)buf;
        b->end = (uint8_t *)buf + nsize;
        b->pos = (uint8_t *)buf;
        b->last = (uint8_t *)buf + size + len;
    }
}
