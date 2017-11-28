//
//  log_builder.c
//  log-c-sdk-new
//
//  Created by 王佳玮 on 16/8/24.
//  Copyright © 2016年 wangjwchn. All rights reserved.
//

#include "log_builder.h"
#include "lz4.h"

log_group_builder* log_group_create()
{
    apr_pool_t *tmp;
    apr_pool_create(&tmp, NULL);
    log_group_builder* bder = (log_group_builder*)apr_palloc(tmp,sizeof(log_group_builder));
    bder->root = tmp;
    bder->grp = (log_group*)apr_palloc(bder->root,sizeof(log_group));
    *bder->grp = (log_group)log_group_init;
    bder->loggroup_size = sizeof(log_group) + sizeof(log_group_builder);
    return bder;
}

void log_group_destroy(log_group_builder* bder)
{
    apr_pool_destroy(bder->root);
}

void add_log(log_group_builder* bder)
{
    if(!bder->grp->logs){
        bder->grp->logs = (log_lg**)apr_palloc(bder->root,sizeof(log_lg*)*INIT_LOG_NUMBER_IN_LOGGROUP);
        bder->grp->n_logs = 0;
    }
    bder->grp->logs[bder->grp->n_logs] = (log_lg*)apr_palloc(bder->root,sizeof(log_lg));
    bder->loggroup_size += sizeof(log_lg);
    *bder->grp->logs[bder->grp->n_logs] = (log_lg)log_lg_init;
    bder->lg = bder->grp->logs[bder->grp->n_logs];
    bder->grp->n_logs++;
    if((bder->grp->n_logs & (bder->grp->n_logs - 1)) == 0
       &&bder->grp->n_logs>=INIT_LOG_NUMBER_IN_LOGGROUP){
        log_lg** tmp = apr_palloc(bder->root,sizeof(log_lg*)*(bder->grp->n_logs<<1));
        memcpy(tmp, bder->grp->logs,sizeof(log_lg*)*bder->grp->n_logs);
        bder->grp->logs = tmp;  //memory usage issue
    }
}

void add_log_time(log_group_builder* bder,uint32_t time)
{
    bder->lg->time = (uint32_t)time;
}

void add_source(log_group_builder* bder,const char* src,size_t len)
{
    char* source = (char*)apr_palloc(bder->root, sizeof(char)*(len+1));
    bder->loggroup_size += sizeof(char)*(len+1) + 2;
    memcpy(source, src, len);
    source[len] = '\0';
    bder->grp->source = source;
}

void add_topic(log_group_builder* bder,const char* tpc,size_t len)
{
    char* topic = (char*)apr_palloc(bder->root, sizeof(char)*(len+1));
    bder->loggroup_size += sizeof(char)*(len+1) + 2;
    memcpy(topic, tpc, len);
    topic[len] = '\0';
    bder->grp->topic = topic;
}

void add_pack_id(log_group_builder* bder, const char* pack, size_t pack_len, size_t packNum)
{
    char packStr[128];
    packStr[127] = '\0';
    snprintf(packStr, 127, "%s-%X", pack, (unsigned int)packNum);
    add_tag(bder, "__pack_id__", strlen("__pack_id__"), packStr, strlen(packStr));
}

void add_tag(log_group_builder* bder,const char* k,size_t k_len,const char* v,size_t v_len)
{
    char* key   = (char*)apr_palloc(bder->root, sizeof(char)*(k_len+1));
    bder->loggroup_size += sizeof(char)*(k_len+1);
    char* value = (char*)apr_palloc(bder->root, sizeof(char)*(v_len+1));
    bder->loggroup_size += sizeof(char)*(v_len+1);
    bder->loggroup_size += 5;
    memcpy(key  , k, k_len);
    memcpy(value, v, v_len);
    key[k_len]   = '\0';
    value[v_len] = '\0';

    log_tag* tag = (log_tag*)apr_palloc(bder->root,sizeof(log_tag));
    bder->loggroup_size += sizeof(log_tag);
    *tag = (log_tag)log_tag_init;
    tag->key = key;
    tag->value = value;

    if(!bder->grp->logtags){
        bder->grp->logtags = (log_tag**)apr_palloc(bder->root,sizeof(log_tag*)*INIT_TAG_NUMBER_IN_LOGGROUP);
        bder->loggroup_size += sizeof(log_tag*)*INIT_TAG_NUMBER_IN_LOGGROUP;
        bder->grp->n_logtags = 0;
    }
    bder->grp->logtags[bder->grp->n_logtags] = tag;
    bder->grp->n_logtags++;
    if((bder->grp->n_logtags & (bder->grp->n_logtags- 1)) == 0
       && bder->grp->n_logtags>=INIT_TAG_NUMBER_IN_LOGGROUP){
        log_tag** tmp = (log_tag**)apr_palloc(bder->root,sizeof(log_tag*)*(bder->grp->n_logtags << 1));
        bder->loggroup_size += sizeof(log_tag*)*(bder->grp->n_logtags << 1);
        memcpy(tmp, bder->grp->logtags,sizeof(log_tag*)*bder->grp->n_logtags);
        bder->grp->logtags = tmp;  //memory usage issue
    }
}

void add_log_key_value(log_group_builder* bder,const char* k,size_t k_len,const char* v,size_t v_len)
{
    char* key   = (char*)apr_palloc(bder->root, sizeof(char)*(k_len+1));
    bder->loggroup_size += sizeof(char)*(k_len+1);
    char* value = (char*)apr_palloc(bder->root, sizeof(char)*(v_len+1));
    bder->loggroup_size += sizeof(char)*(v_len+1);
    bder->loggroup_size += 5;

    memcpy(key  , k, k_len);
    memcpy(value, v, v_len);
    key[k_len]   = '\0';
    value[v_len] = '\0';
    
    log_content* con = (log_content*)apr_palloc(bder->root,sizeof(log_content));
    *con = (log_content)log_content_init;
    con->key = key;
    con->value = value;
    
    if(!bder->lg->contents){
        time_t rawtime;
        time(&rawtime);
        bder->lg->time = (uint32_t)rawtime;
        bder->lg->contents = (log_content**)apr_palloc(bder->root,sizeof(log_content*)*INIT_KVPAIR_NUMBER_IN_LOG);
        bder->lg->n_contents = 0;
    }
    bder->lg->contents[bder->lg->n_contents] = con;
    bder->lg->n_contents++;
    if((bder->lg->n_contents & (bder->lg->n_contents - 1)) == 0
       &&bder->lg->n_contents>=INIT_KVPAIR_NUMBER_IN_LOG){
        log_content** tmp = apr_palloc(bder->root,sizeof(log_content*)*(bder->lg->n_contents<<1));
        memcpy(tmp, bder->lg->contents,sizeof(log_content*)*bder->lg->n_contents);
        bder->lg->contents = tmp;  //memory usage issue
        
    }
}

log_buf* serialize_to_proto_buf(log_group_builder* bder){
    size_t length = log_get_packed_size(bder->grp);
    log_buf* buf = (log_buf*)apr_palloc(bder->root, sizeof(log_buf));
    buf->length = length;
    buf->data = (char*)apr_palloc(bder->root, length);
    log_pack(bder->grp, buf->data);
    return buf;
}

log_buf* serialize_to_proto_buf_with_malloc(log_group_builder* bder)
{
    size_t length = log_get_packed_size(bder->grp);
    log_buf* buf = (log_buf*)malloc(sizeof(log_buf));
    buf->length = length;
    buf->data = (char*)malloc(length);
    log_pack(bder->grp, buf->data);
    return buf;
}

lz4_log_buf* serialize_to_proto_buf_with_malloc_lz4(log_group_builder* bder)
{
    size_t length = log_get_packed_size(bder->grp);
    unsigned char * buf = (unsigned char *)malloc(length);
    log_pack(bder->grp, buf);
    int compress_bound = LZ4_compressBound(length);
    char *compress_data = (char *)malloc(compress_bound);
    int compressed_size = LZ4_compress_default((char *)buf, compress_data, length, compress_bound);
    if(compressed_size <= 0)
    {
        free(buf);
        free(compress_data);
        return NULL;
    }
    lz4_log_buf* pLogbuf = (lz4_log_buf*)malloc(sizeof(lz4_log_buf) + compressed_size);
    pLogbuf->length = compressed_size;
    pLogbuf->raw_length = length;
    memcpy(pLogbuf->data, compress_data, compressed_size);
    free(buf);
    free(compress_data);
    return pLogbuf;
}

void free_proto_log_buf(log_buf* pBuf)
{
    free(pBuf->data);
    free(pBuf);
}

void free_lz4_log_buf(lz4_log_buf* pBuf)
{
    free(pBuf);
}

