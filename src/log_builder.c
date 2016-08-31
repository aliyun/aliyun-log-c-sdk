//
//  log_builder.c
//  log-c-sdk-new
//
//  Created by 王佳玮 on 16/8/24.
//  Copyright © 2016年 wangjwchn. All rights reserved.
//

#include "log_builder.h"

log_group_builder* log_group_create()
{
    apr_pool_t *tmp;
    apr_pool_create(&tmp, NULL);
    log_group_builder* bder = (log_group_builder*)apr_palloc(tmp,sizeof(log_group_builder));
    bder->root = tmp;
    bder->grp = (log_group*)apr_palloc(bder->root,sizeof(log_group));
    *bder->grp = (log_group)log_group_init;
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
    *bder->grp->logs[bder->grp->n_logs] = (log_lg)log_lg_init;
    bder->lg = bder->grp->logs[bder->grp->n_logs];
    bder->grp->n_logs++;
    if((bder->grp->n_logs & bder->grp->n_logs - 1) == 0
       &&bder->grp->n_logs>=INIT_LOG_NUMBER_IN_LOGGROUP){
        log_lg** tmp = apr_palloc(bder->root,sizeof(log_lg*)*bder->grp->n_logs<<1);
        memcpy(tmp, bder->grp->logs,sizeof(log_lg*)*bder->grp->n_logs);
        bder->grp->logs = tmp;  //memory usage issue
    }
}

void add_log_time(log_group_builder* bder,uint32_t time)
{
    bder->lg->time = (uint32_t)time;
}

void add_source(log_group_builder* bder,char* src,size_t len)
{
    char* source = (char*)apr_palloc(bder->root, sizeof(char)*(len+1));
    memcpy(source, src, len);
    source[len] = '\0';
    bder->grp->source = source;
}

void add_topic(log_group_builder* bder,char* tpc,size_t len)
{
    char* topic = (char*)apr_palloc(bder->root, sizeof(char)*(len+1));
    memcpy(topic, tpc, len);
    topic[len] = '\0';
    bder->grp->topic = topic;
}

void lg_end(log_group_builder* bder)
{
    if(!bder->grp->logs){
        bder->grp->logs = (log_lg**)apr_palloc(bder->root,sizeof(log_lg*)*INIT_LOG_NUMBER_IN_LOGGROUP);
        bder->grp->n_logs = 0;
    }
    bder->grp->logs[bder->grp->n_logs] = bder->lg;
    bder->grp->n_logs++;
    if((bder->grp->n_logs & bder->grp->n_logs - 1) == 0
       &&bder->grp->n_logs>=INIT_LOG_NUMBER_IN_LOGGROUP){
        log_lg** tmp = apr_palloc(bder->root,sizeof(log_lg*)*bder->grp->n_logs<<1);
        memcpy(tmp, bder->grp->logs,sizeof(log_lg*)*bder->grp->n_logs);
        bder->grp->logs = tmp;  //memory usage issue
    }
    bder->lg = (log_lg*)apr_palloc(bder->root,sizeof(log_lg));
    *bder->lg = (log_lg)log_lg_init;
}

void add_log_key_value(log_group_builder* bder,char* k,size_t v_len,char* v,size_t k_len)
{
    char* key   = (char*)apr_palloc(bder->root, sizeof(char)*(k_len+1));
    char* value = (char*)apr_palloc(bder->root, sizeof(char)*(v_len+1));
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
    if((bder->lg->n_contents & bder->lg->n_contents - 1) == 0
       &&bder->lg->n_contents>=INIT_KVPAIR_NUMBER_IN_LOG){
        log_content** tmp = apr_palloc(bder->root,sizeof(log_content*)*bder->lg->n_contents<<1);
        memcpy(tmp, bder->lg->contents,sizeof(log_content*)*bder->lg->n_contents);
        bder->lg->contents = tmp;  //memory usage issue
        
    }
}

log_buf* serialize_to_proto_buf(log_group_builder* bder){
    log_buf* buf = (log_buf*)apr_palloc(bder->root, sizeof(log_buf));
    
    buf->length = log_get_packed_size(bder->grp);
    buf->data = apr_palloc(bder->root,buf->length);
    log_pack(bder->grp,buf->data);
    return buf;
}

