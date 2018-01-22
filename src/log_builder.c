//
//  log_builder.c
//  log-c-sdk-new
//
//  Created by 王佳玮 on 16/8/24.
//  Copyright © 2016年 wangjwchn. All rights reserved.
//

#include "log_builder.h"
#include "lz4.h"
#include <string.h>
#include <stdio.h>

log_group_builder* log_group_create()
{
    log_group_builder* bder = (log_group_builder*)malloc(sizeof(log_group_builder)+sizeof(log_group));
    bder->grp = (log_group*)((char *)(bder) + sizeof(log_group_builder));
    *bder->grp = (log_group)log_group_init;
    bder->loggroup_size = sizeof(log_group) + sizeof(log_group_builder);
    bder->builder_time = time(NULL);
    return bder;
}

void log_group_destroy(log_group_builder* bder)
{
    // free tag
    size_t i = 0;
    log_group* group = bder->grp;
    if (group->logtags != NULL)
    {
        for (i = 0; i < group->n_logtags; ++i)
        {
            // use 1 free
            //free(group->logtags[i]->key);
            //free(group->logtags[i]->value);
            free(group->logtags[i]);
        }
        free(group->logtags);
    }
    // free log
    if (group->logs != NULL)
    {
        for (i = 0; i < group->n_logs; ++i)
        {
            log_lg * log = group->logs[i];
            if (log->contents != NULL)
            {
                //size_t j = 0;
                //for (; j < log->n_contents; ++j)
                //{
                //    // use 1 free
                //    //free(log->contents[j]->key);
                //    //free(log->contents[j]->value);
                //    free(log->contents[j]);
                //}
                free(log->contents);
            }
            free(log);
        }
        free(group->logs);
    }
    if (group->topic != NULL)
    {
        free(group->topic);
    }
    if (group->source != NULL)
    {
        free(group->source);
    }
    if (group->category != NULL)
    {
        free(group->category);
    }
    if (group->machineuuid != NULL)
    {
        free(group->machineuuid);
    }
    // free self
    free(bder);
}

void _add_log(log_group_builder* bder)
{
    if(!bder->grp->logs){
        bder->grp->logs = (log_lg**)malloc(sizeof(log_lg*)*INIT_LOG_NUMBER_IN_LOGGROUP);
        bder->grp->n_logs = 0;
    }
    bder->grp->logs[bder->grp->n_logs] = (log_lg*)malloc(sizeof(log_lg));
    bder->loggroup_size += sizeof(log_lg);
    *bder->grp->logs[bder->grp->n_logs] = (log_lg)log_lg_init;
    bder->lg = bder->grp->logs[bder->grp->n_logs];
    bder->grp->n_logs++;
    if((bder->grp->n_logs & (bder->grp->n_logs - 1)) == 0
       &&bder->grp->n_logs>=INIT_LOG_NUMBER_IN_LOGGROUP){
        log_lg** tmp = malloc(sizeof(log_lg*)*(bder->grp->n_logs<<1));
        memcpy(tmp, bder->grp->logs,sizeof(log_lg*)*bder->grp->n_logs);
        free(bder->grp->logs);
        bder->grp->logs = tmp;
    }
}

void add_log_full(log_group_builder* bder, int32_t pair_count, char ** keys, size_t * key_lens, char ** values, size_t * val_lens)
{
    // use only 1 malloc
    _add_log(bder);
    int32_t i = 0;
    int32_t totalBufferSize = pair_count * (sizeof(log_content*) + 2 * sizeof(char) + sizeof(log_content));
    for (; i < pair_count; ++i)
    {
        totalBufferSize += sizeof(char) * (key_lens[i] + val_lens[i]);
    }
    void * totalBuf = malloc(totalBufferSize);
    bder->loggroup_size += totalBufferSize;
    log_lg * log = bder->lg;
    log->n_contents = pair_count;
    log->contents = (log_content **)totalBuf;
    log_content * contents = (log_content *)((uint8_t *)totalBuf + pair_count * (sizeof(log_content*)));
    char * keyValBuf = (char *)((uint8_t *)totalBuf + pair_count * (sizeof(log_content*) + sizeof(log_content)));
    for (i = 0; i < pair_count; ++i)
    {
        log->contents[i] = &contents[i];
        contents[i] = (log_content)log_content_init;
        contents[i].key = keyValBuf;
        memcpy(contents[i].key, keys[i], key_lens[i]);
        contents[i].key[key_lens[i]] = '\0';
        keyValBuf += key_lens[i] + 1;
        contents[i].value = keyValBuf;
        memcpy(contents[i].value, values[i], val_lens[i]);
        contents[i].value[val_lens[i]] = '\0';
        keyValBuf += val_lens[i] + 1;
    }
}

void add_log_time(log_group_builder* bder,uint32_t time)
{
    if (time < 1263563523)
    {
        time = 1263563523;
    }
    bder->lg->time = (uint32_t)time;
}

void add_source(log_group_builder* bder,const char* src,size_t len)
{
    char* source = (char*)malloc( sizeof(char)*(len+1));
    bder->loggroup_size += sizeof(char)*(len+1) + 2;
    memcpy(source, src, len);
    source[len] = '\0';
    bder->grp->source = source;
}

void add_topic(log_group_builder* bder,const char* tpc,size_t len)
{
    char* topic = (char*)malloc(sizeof(char)*(len+1));
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
    // use only 1 malloc
    char * totalBuf = (char*)malloc(sizeof(char)*(k_len+2+v_len+sizeof(log_tag)));
    char* key = totalBuf + sizeof(log_tag);
    bder->loggroup_size += sizeof(char)*(k_len+1);
    char* value = key+k_len+1;
    bder->loggroup_size += sizeof(char)*(v_len+1);
    bder->loggroup_size += 5;
    memcpy(key, k, k_len);
    memcpy(value, v, v_len);
    key[k_len]   = '\0';
    value[v_len] = '\0';

    log_tag* tag = (log_tag*)totalBuf;
    bder->loggroup_size += sizeof(log_tag);
    *tag = (log_tag)log_tag_init;
    tag->key = key;
    tag->value = value;

    if(!bder->grp->logtags){
        bder->grp->logtags = (log_tag**)malloc(sizeof(log_tag*)*INIT_TAG_NUMBER_IN_LOGGROUP);
        bder->loggroup_size += sizeof(log_tag*)*INIT_TAG_NUMBER_IN_LOGGROUP;
        bder->grp->n_logtags = 0;
    }
    bder->grp->logtags[bder->grp->n_logtags] = tag;
    bder->grp->n_logtags++;
    if((bder->grp->n_logtags & (bder->grp->n_logtags- 1)) == 0
       && bder->grp->n_logtags>=INIT_TAG_NUMBER_IN_LOGGROUP){
        log_tag** tmp = (log_tag**)malloc(sizeof(log_tag*)*(bder->grp->n_logtags << 1));
        bder->loggroup_size += sizeof(log_tag*)*(bder->grp->n_logtags << 1);
        memcpy(tmp, bder->grp->logtags,sizeof(log_tag*)*bder->grp->n_logtags);
        free(bder->grp->logtags);
        bder->grp->logtags = tmp;
    }
}

/*
void add_log_key_value(log_group_builder* bder,const char* k,size_t k_len,const char* v,size_t v_len)
{
    // use 1 malloc
    char * totalBuf = (char*)malloc(sizeof(char)*(k_len+2+v_len+sizeof(log_content)));
    char* key   = totalBuf + sizeof(log_content);
    bder->loggroup_size += sizeof(char)*(k_len+1);
    char* value = key+k_len+1;
    bder->loggroup_size += sizeof(char)*(v_len+1);
    bder->loggroup_size += 5;

    memcpy(key  , k, k_len);
    memcpy(value, v, v_len);
    key[k_len]   = '\0';
    value[v_len] = '\0';
    
    log_content* con = (log_content*)totalBuf;
    *con = (log_content)log_content_init;
    con->key = key;
    con->value = value;
    
    if(!bder->lg->contents){
        time_t rawtime;
        time(&rawtime);
        bder->lg->time = (uint32_t)rawtime;
        bder->lg->contents = (log_content**)malloc(sizeof(log_content*)*INIT_KVPAIR_NUMBER_IN_LOG);
        bder->lg->n_contents = 0;
    }
    bder->lg->contents[bder->lg->n_contents] = con;
    bder->lg->n_contents++;
    if((bder->lg->n_contents & (bder->lg->n_contents - 1)) == 0
       &&bder->lg->n_contents>=INIT_KVPAIR_NUMBER_IN_LOG){
        log_content** tmp = malloc(sizeof(log_content*)*(bder->lg->n_contents<<1));
        memcpy(tmp, bder->lg->contents,sizeof(log_content*)*bder->lg->n_contents);
        free(bder->lg->contents);
        bder->lg->contents = tmp;  //memory usage issue
        
    }
}
 */


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

void free_lz4_log_buf(lz4_log_buf* pBuf)
{
    free(pBuf);
}

