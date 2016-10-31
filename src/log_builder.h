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
#include "log_logstores.pb-c.h"

#define INIT_KVPAIR_NUMBER_IN_LOG        8
#define INIT_LOG_NUMBER_IN_LOGGROUP      8
#define log_group                 LogLogstores__LogGroup
#define log_group_init            LOG_LOGSTORES__LOG_GROUP__INIT
#define log_content               LogLogstores__Log__Content
#define log_content_init          LOG_LOGSTORES__LOG__CONTENT__INIT
#define log_lg                    LogLogstores__Log
#define log_lg_init               LOG_LOGSTORES__LOG__INIT
#define log_get_packed_size       log_logstores__log_group__get_packed_size
#define log_pack                  log_logstores__log_group__pack

#define _new(type, ...) ({\
type* __t = (type*) malloc(sizeof(type));\
*__t = (type){__VA_ARGS__};\
__t; })

typedef struct _log_buf{
    void* data;
    size_t length;
}log_buf;

typedef struct _log_group_builder{
    apr_pool_t *root;
    log_group* grp;
    log_lg* lg;
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
extern log_group_builder* log_group_create();
extern void log_group_destroy(log_group_builder* bder);
extern void add_log(log_group_builder* bder);
extern void* pack_protobuf(log_group_builder* bder);
extern void add_source(log_group_builder* bder,const char* src,size_t len);
extern void add_topic(log_group_builder* bder,const char* tpc,size_t len);
extern void add_log_time(log_group_builder* bder,uint32_t time);
extern void add_log_key_value(log_group_builder* bder,const char* k,size_t k_len,const char* v,size_t v_len);


#endif /* log_builder_h */
