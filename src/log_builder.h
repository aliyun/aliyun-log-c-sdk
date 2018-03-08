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
#include <apr_time.h>
#include <apr_strings.h>
#include <apr_pools.h>
#include <apr_tables.h>
#include <apr_file_io.h>

#include "log_define.h"
#include "log_logstores.pb-c.h"
LOG_CPP_START

#define INIT_KVPAIR_NUMBER_IN_LOG        8
#define INIT_LOG_NUMBER_IN_LOGGROUP      8
#define INIT_TAG_NUMBER_IN_LOGGROUP      4
#define log_group                 SlsLogs__LogGroup
#define log_group_init            SLS_LOGS__LOG_GROUP__INIT
#define log_group_free         sls_logs__log_group__free_unpacked
#define log_content               SlsLogs__Log__Content
#define log_content_init          SLS_LOGS__LOG__CONTENT__INIT
#define log_lg                    SlsLogs__Log
#define log_lg_init               SLS_LOGS__LOG__INIT
#define log_get_packed_size       sls_logs__log_group__get_packed_size
#define log_pack                  sls_logs__log_group__pack
#define log_tag                   SlsLogs__LogTag
#define log_tag_init              SLS_LOGS__LOG_TAG__INIT

#define _new(type, ...) ({\
type* __t = (type*) malloc(sizeof(type));\
*__t = (type){__VA_ARGS__};\
__t; })

typedef struct _log_buf{
    void * data;
    size_t length;
}log_buf;

typedef struct _lz4_log_buf{
    size_t length;
    size_t raw_length;
    unsigned char data[0];
}lz4_log_buf;

typedef struct _log_group_builder{
    apr_pool_t *root;
    log_group* grp;
    log_lg* lg;
    size_t loggroup_size;
    uint32_t builder_time;
    void * private_value;
}log_group_builder;

typedef struct _log_header_pair{
    char* header_key;
    char* header_value;
} log_header_pair;

typedef struct _log_http_cont{
    char* url;
    apr_table_t *headers;
    log_buf body;
    apr_pool_t *root;
} log_http_cont;


extern log_buf* serialize_to_proto_buf(log_group_builder* bder);
extern log_buf* serialize_to_proto_buf_with_malloc(log_group_builder* bder);
extern void free_proto_log_buf(log_buf* pBuf);
extern log_group * deserialize_from_lz4_proto_buf(const unsigned char * buffer, size_t buf_size, size_t raw_buf_size);
extern lz4_log_buf* serialize_to_proto_buf_with_malloc_lz4(log_group_builder* bder);
extern void free_lz4_log_buf(lz4_log_buf* pBuf);
extern log_group_builder* log_group_create();
extern void log_group_destroy(log_group_builder* bder);
extern void add_log(log_group_builder* bder);
extern void* pack_protobuf(log_group_builder* bder);
extern void add_source(log_group_builder* bder,const char* src,size_t len);
extern void add_topic(log_group_builder* bder,const char* tpc,size_t len);
extern void add_tag(log_group_builder* bder,const char* k,size_t k_len,const char* v,size_t v_len);
extern void add_pack_id(log_group_builder* bder, const char* pack, size_t pack_len, size_t packNum);
extern void add_log_time(log_group_builder* bder,uint32_t time);
extern void add_log_key_value(log_group_builder* bder,const char* k,size_t k_len,const char* v,size_t v_len);


LOG_CPP_END
#endif /* log_builder_h */
