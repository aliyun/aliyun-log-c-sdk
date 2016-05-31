#ifndef LIBAOS_BUF_H
#define LIBAOS_BUF_H

#include "aos_define.h"
#include "aos_list.h"

AOS_CPP_START

typedef struct {
    aos_list_t node;
    uint8_t *pos;
    uint8_t *last;
    uint8_t *start;
    uint8_t *end;
} aos_buf_t;

typedef struct {
    aos_list_t node;
    int64_t file_pos;
    int64_t file_last;
    apr_file_t *file;
    uint32_t owner:1;
} aos_file_buf_t;

aos_buf_t *aos_create_buf(aos_pool_t *p, int size);
#define aos_buf_size(b) (b->last - b->pos)

aos_file_buf_t *aos_create_file_buf(aos_pool_t *p);

aos_buf_t *aos_buf_pack(aos_pool_t *p, const void *data, int size);

int64_t aos_buf_list_len(aos_list_t *list);

char *aos_buf_list_content(aos_pool_t *p, aos_list_t *list);

void aos_buf_append_string(aos_pool_t *p, aos_buf_t *b, const char *str, int len);

/**
 * @param fb file_pos, file_last equal file_size.
 * @return AOSE_OK success, other failure.
 */ 
int aos_open_file_for_read(aos_pool_t *p, const char *path, aos_file_buf_t *fb);

int aos_open_file_for_all_read(aos_pool_t *p, const char *path, aos_file_buf_t *fb);

int aos_open_file_for_range_read(aos_pool_t *p, const char *path, 
                                 int64_t file_pos, int64_t file_last, 
                                 aos_file_buf_t *fb);

/**
 * create the file if not there, truncate if file exists. 
 * @param fb not check file_pos, file_last.
 * @return AOSE_OK success, other failure.
 */
int aos_open_file_for_write(aos_pool_t *p, const char *path, aos_file_buf_t *fb);

AOS_CPP_END

#endif

