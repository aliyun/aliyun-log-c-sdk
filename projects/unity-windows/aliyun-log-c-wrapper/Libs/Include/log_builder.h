//
//  log_builder.h
//  log-c-sdk
//
//  Created by 王佳玮 on 16/8/23.
//  Copyright © 2016年 wangjwchn. All rights reserved.
//
#ifndef LOG_BUILDER_H
#define LOG_BUILDER_H

#include <stdlib.h>
#include <time.h>
#include <stdint.h>

#include "log_define.h"
LOG_CPP_START

typedef struct _log_tag{
    char * buffer;
    char * now_buffer;
    uint32_t max_buffer_len;
    uint32_t now_buffer_len;
}log_tag;


typedef struct _log_group{
    char * source;
    char * topic;
    log_tag tags;
    log_tag logs;
    size_t n_logs;
#ifdef LOG_KEY_VALUE_FLAG
    char * log_now_buffer;
#endif
}log_group;

typedef struct _lz4_log_buf{
    size_t length;
    size_t raw_length;
    unsigned char data[0];
}lz4_log_buf;

typedef struct _log_group_builder{
    log_group* grp;
    size_t loggroup_size;
    void * private_value;
    uint32_t builder_time;
}log_group_builder;

typedef struct _log_buffer {
    char * buffer;
    uint32_t n_buffer;
}log_buf;

extern log_buf serialize_to_proto_buf_with_malloc(log_group_builder* bder);
extern lz4_log_buf* serialize_to_proto_buf_with_malloc_lz4(log_group_builder* bder);
extern lz4_log_buf* serialize_to_proto_buf_with_malloc_no_lz4(log_group_builder* bder);
extern void free_lz4_log_buf(lz4_log_buf* pBuf);
extern log_group_builder* log_group_create();
extern void log_group_destroy(log_group_builder* bder);
extern void add_log_full(log_group_builder* bder, uint32_t logTime, int32_t pair_count, char ** keys, size_t * key_lens, char ** values, size_t * val_lens);
extern void add_source(log_group_builder* bder,const char* src,size_t len);
extern void add_topic(log_group_builder* bder,const char* tpc,size_t len);
extern void add_tag(log_group_builder* bder,const char* k,size_t k_len,const char* v,size_t v_len);
extern void add_pack_id(log_group_builder* bder, const char* pack, size_t pack_len, size_t packNum);
extern void fix_log_group_time(char * pb_buffer, size_t len, uint32_t new_time);

// if you want to use this functions, add `-DADD_LOG_KEY_VALUE_FUN=ON` for cmake. eg `cmake . -DADD_LOG_KEY_VALUE_FUN=ON`
#ifdef LOG_KEY_VALUE_FLAG
/**
 * call it when you want to add a new log
 * @note sequence must be : add_log_begin -> add_log_time/add_log_key_value..... -> add_log_end
 * @param bder
 */
extern void add_log_begin(log_group_builder * bder);

/**
 * set log's time, must been called only once in one log
 * @param bder
 * @param logTime
 */
extern void add_log_time(log_group_builder * bder, uint32_t logTime);

/**
 * add key&value pair to log tail
 * @param bder
 * @param key
 * @param key_len
 * @param value
 * @param value_len
 */
extern void add_log_key_value(log_group_builder *bder, char * key, size_t key_len, char * value, size_t value_len);

/**
 * add log end, call it when you add time and key&value done
 * @param bder
 */
extern void add_log_end(log_group_builder * bder);

#endif

LOG_CPP_END
#endif /* log_builder_h */
