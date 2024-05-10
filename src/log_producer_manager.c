//
// Created by ZhangCheng on 20/11/2017.
//

#include "log_producer_manager.h"
#include "inner_log.h"
#include "md5.h"
#include "sds.h"

#define LOG_PRODUCER_FLUSH_INTERVAL_MS 100


#define MAX_LOGGROUP_QUEUE_SIZE 102400
#define MIN_LOGGROUP_QUEUE_SIZE 32

#define MAX_MANAGER_FLUSH_COUNT 100  // 10MS * 100
#define MAX_SENDER_FLUSH_COUNT 100 // 10ms * 100

log_queue * g_sender_data_queue = NULL;
THREAD * g_send_threads = NULL;
int32_t g_send_thread_count = 0;
volatile uint8_t g_send_thread_destroy = 0;

void * log_producer_send_thread(void * param);

char * _get_pack_id(const char * configName, const char * ip)
{
    sds buf = sdsnew(configName);
    buf = sdscat(buf, ip);
    char timeAndRandomBuf[9];
    uint32_t now_time = time(NULL);
    timeAndRandomBuf[0] = now_time & 0xFF;
    timeAndRandomBuf[1] = (now_time >> 8 ) & 0xFF;
    timeAndRandomBuf[2] = (now_time >> 16 ) & 0xFF;
    timeAndRandomBuf[3] = (now_time >> 24 ) & 0xFF;
    timeAndRandomBuf[4] = rand() % 128;
    timeAndRandomBuf[5] = rand() % 128;
    timeAndRandomBuf[6] = rand() % 128;
    timeAndRandomBuf[7] = rand() % 128;
    timeAndRandomBuf[8] = '\0';
    buf = sdscat(buf, timeAndRandomBuf);
    unsigned char md5Buf[16];
    mbedtls_md5((const unsigned char *)buf, sdslen(buf), md5Buf);
    sdsfree(buf);
    int loop = 0;
    char * val = (char *)malloc(sizeof(char) * 32);
    memset(val, 0, sizeof(char) * 32);
    for(; loop < 8; ++loop)
    {
        unsigned char a = ((md5Buf[loop])>>4) & 0xF, b = (md5Buf[loop]) & 0xF;
        val[loop<<1] = a > 9 ? (a - 10 + 'A') : (a + '0');
        val[(loop<<1)|1] = b > 9 ? (b - 10 + 'A') : (b + '0');
    }
    return val;
}

void _try_flush_loggroup(log_producer_manager * producer_manager)
{
    int32_t now_time = time(NULL);

    CS_ENTER(producer_manager->lock);
    if (producer_manager->builder != NULL && now_time - producer_manager->firstLogTime > producer_manager->producer_config->packageTimeoutInMS / 1000)
    {
        log_group_builder * builder = producer_manager->builder;
        producer_manager->builder = NULL;
        CS_LEAVE(producer_manager->lock);

        size_t loggroup_size = builder->loggroup_size;
        int rst = log_queue_push(producer_manager->loggroup_queue, builder);
        aos_debug_log("try push loggroup to flusher, size : %d, status : %d", (int)loggroup_size, rst);
        if (rst != 0)
        {
            aos_error_log("try push loggroup to flusher failed, force drop this log group, error code : %d", rst);
            log_group_destroy(builder);
        }
        else
        {
            producer_manager->totalBufferSize += loggroup_size;
            COND_SIGNAL(producer_manager->triger_cond);
        }
    }
    else
    {
        CS_LEAVE(producer_manager->lock);
    }
}

void * log_producer_flush_thread(void * param)
{
    log_producer_manager * root_producer_manager = (log_producer_manager*)param;
    aos_info_log("start run flusher thread, config : %s", root_producer_manager->producer_config->logstore);
    while (root_producer_manager->shutdown == 0)
    {

        CS_ENTER(root_producer_manager->lock);
        COND_WAIT_TIME(root_producer_manager->triger_cond,
                       root_producer_manager->lock,
                       LOG_PRODUCER_FLUSH_INTERVAL_MS);
        CS_LEAVE(root_producer_manager->lock);


//        aos_debug_log("run flusher thread, config : %s, now loggroup size : %d, delta time : %d",
//                      producer_manager->producer_config->configName,
//                      producer_manager->builder != NULL ? (int)producer_manager->builder->loggroup_size : 0,
//                      (int)(now_time - producer_manager->firstLogTime));

        // try read queue
        do
        {
            // if send queue is full, skip pack and send data
            if (root_producer_manager->send_param_queue_write - root_producer_manager->send_param_queue_read >= root_producer_manager->send_param_queue_size)
            {
                break;
            }
            void * data = log_queue_trypop(root_producer_manager->loggroup_queue);
            if (data != NULL)
            {
                // process data
                log_group_builder * builder = (log_group_builder*)data;

                log_producer_manager * producer_manager = (log_producer_manager *)builder->private_value;
                CS_ENTER(root_producer_manager->lock);
                producer_manager->totalBufferSize -= builder->loggroup_size;
                CS_LEAVE(root_producer_manager->lock);


                log_producer_config * config = producer_manager->producer_config;
                int i = 0;
                for (i = 0; i < config->tagCount; ++i)
                {
                    add_tag(builder, config->tags[i].key, strlen(config->tags[i].key), config->tags[i].value, strlen(config->tags[i].value));
                }
                if (config->topic != NULL)
                {
                    add_topic(builder, config->topic, strlen(config->topic));
                }
                if (producer_manager->source != NULL)
                {
                    add_source(builder, producer_manager->source, strlen(producer_manager->source));
                }
                if (producer_manager->pack_prefix != NULL)
                {
                    add_pack_id(builder, producer_manager->pack_prefix, strlen(producer_manager->pack_prefix), producer_manager->pack_index++);
                }

                lz4_log_buf * lz4_buf = serialize_to_log_buf_with_malloc(builder, config->compressType);

                if (lz4_buf == NULL)
                {
                    aos_error_log("serialize loggroup to proto buf with lz4 failed");
                }
                else
                {
                    CS_ENTER(root_producer_manager->lock);
                    producer_manager->totalBufferSize += lz4_buf->length;
                    CS_LEAVE(root_producer_manager->lock);

                    aos_debug_log("push loggroup to sender, config %s, loggroup size %d, lz4 size %d, now buffer size %d",
                                  config->logstore, (int)lz4_buf->raw_length, (int)lz4_buf->length, (int)producer_manager->totalBufferSize);
                    // if use multi thread, should change producer_manager->send_pool to NULL
                    //apr_pool_t * pool = config->sendThreadCount == 1 ? producer_manager->send_pool : NULL;
                    log_producer_send_param * send_param = create_log_producer_send_param(config, producer_manager, lz4_buf, builder->builder_time);
                    root_producer_manager->send_param_queue[root_producer_manager->send_param_queue_write++ % root_producer_manager->send_param_queue_size] = send_param;
                }
                log_group_destroy(builder);
                continue;
            }
            break;
        }while(1);

        // if no job, check now loggroup
        _try_flush_loggroup(root_producer_manager);

        // send data
        // [priority] : producer manager's send thread > global send thread > send directly
        if (root_producer_manager->send_threads != NULL)
        {
            // if send thread count > 0, we just push send_param to sender queue
            while (root_producer_manager->send_param_queue_write > root_producer_manager->send_param_queue_read && !log_queue_isfull(root_producer_manager->sender_data_queue))
            {
                log_producer_send_param * send_param = root_producer_manager->send_param_queue[root_producer_manager->send_param_queue_read++ % root_producer_manager->send_param_queue_size];
                // push always success
                log_queue_push(root_producer_manager->sender_data_queue, send_param);
            }
        }
        else if (g_send_threads != NULL && g_sender_data_queue != NULL)
        {
            // if send thread count > 0, we just push send_param to sender queue
            while (root_producer_manager->send_param_queue_write > root_producer_manager->send_param_queue_read && !log_queue_isfull(g_sender_data_queue))
            {
                (void)ATOMICINT_INC(&root_producer_manager->ref_count);
                assert(root_producer_manager->ref_count > 1);
                log_producer_send_param * send_param = root_producer_manager->send_param_queue[root_producer_manager->send_param_queue_read++ % root_producer_manager->send_param_queue_size];
                // make sure push success
                while(0 != log_queue_push(g_sender_data_queue, send_param))
                {
                    ;
                }
            }
        }
        else if (root_producer_manager->send_param_queue_write > root_producer_manager->send_param_queue_read)
        {
            // if no sender thread, we send this packet out in flush thread
            log_producer_send_param * send_param = root_producer_manager->send_param_queue[root_producer_manager->send_param_queue_read++ % root_producer_manager->send_param_queue_size];
            log_producer_send_data(send_param);
        }
    }
    aos_info_log("exit flusher thread, config : %s", root_producer_manager->producer_config->logstore);
    return NULL;
}

log_producer_manager * create_log_producer_manager(log_producer_config * producer_config)
{
    aos_debug_log("create log producer manager : %s", producer_config->logstore);
    log_producer_manager * producer_manager = (log_producer_manager *)malloc(sizeof(log_producer_manager));
    memset(producer_manager, 0, sizeof(log_producer_manager));

    (void)ATOMICINT_INC(&producer_manager->ref_count);

    assert(producer_manager->ref_count == 1);

    producer_manager->producer_config = producer_config;

    int32_t base_queue_size = producer_config->maxBufferBytes / (producer_config->logBytesPerPackage + 1) + 10;
    if (producer_config->logQueueSize > 0)
    {
        base_queue_size = producer_config->logQueueSize;
    }
    if (base_queue_size < MIN_LOGGROUP_QUEUE_SIZE)
    {
        base_queue_size = MIN_LOGGROUP_QUEUE_SIZE;
    }
    else if (base_queue_size > MAX_LOGGROUP_QUEUE_SIZE)
    {
        base_queue_size = MAX_LOGGROUP_QUEUE_SIZE;
    }

    producer_manager->loggroup_queue = log_queue_create(base_queue_size);
    producer_manager->send_param_queue_size = base_queue_size * 2;
    producer_manager->send_param_queue = malloc(sizeof(log_producer_send_param*) * producer_manager->send_param_queue_size);

    if (producer_config->sendThreadCount > 0)
    {
        producer_manager->send_threads = (THREAD *)malloc(sizeof(THREAD) * producer_config->sendThreadCount);
        producer_manager->sender_data_queue = log_queue_create(base_queue_size * 2);
        int32_t threadId = 0;
        for (; threadId < producer_manager->producer_config->sendThreadCount; ++threadId)
        {
            THREAD_INIT(producer_manager->send_threads[threadId], log_producer_send_thread, producer_manager);
        }
    }


    producer_manager->triger_cond = CreateCond();
    producer_manager->lock = CreateCriticalSection();
    THREAD_INIT(producer_manager->flush_thread, log_producer_flush_thread, producer_manager);

    if (producer_config->source != NULL)
    {
        producer_manager->source = sdsnew(producer_config->source);
    }
    else
    {
        producer_manager->source = sdsnew("undefined");
    }


    producer_manager->pack_prefix = _get_pack_id(producer_config->logstore, producer_manager->source);
    if (producer_manager->pack_prefix == NULL)
    {
        producer_manager->pack_prefix = (char *)malloc(32);
        srand(time(NULL));
        int i = 0;
        for (i = 0; i < 16; ++i)
        {
            producer_manager->pack_prefix[i] = rand() % 10 + '0';
        }
        producer_manager->pack_prefix[i] = '\0';
    }
    return producer_manager;
}


void _push_last_loggroup(log_producer_manager * manager)
{
    CS_ENTER(manager->lock);
    log_group_builder * builder = manager->builder;
    manager->builder = NULL;
    if (builder != NULL)
    {
        size_t loggroup_size = builder->loggroup_size;
        aos_debug_log("try push loggroup to flusher, size : %d, log size %d", (int)builder->loggroup_size, (int)builder->grp->logs.now_buffer_len);
        int32_t status = log_queue_push(manager->loggroup_queue, builder);
        if (status != 0)
        {
            aos_error_log("try push loggroup to flusher failed, force drop this log group, error code : %d", status);
            log_group_destroy(builder);
        }
        else
        {
            manager->totalBufferSize += loggroup_size;
            COND_SIGNAL(manager->triger_cond);
        }
    }
    CS_LEAVE(manager->lock);
}

void destroy_log_producer_manager_tail(log_producer_manager * manager)
{
    aos_info_log("delete producer manager tail");
    DeleteCriticalSection(manager->lock);
    if (manager->pack_prefix != NULL)
    {
        free(manager->pack_prefix);
    }
    if (manager->send_param_queue != NULL)
    {
        free(manager->send_param_queue);
    }
    sdsfree(manager->source);
    destroy_log_producer_config(manager->producer_config);
    free(manager);
}

void destroy_log_producer_manager(log_producer_manager * manager)
{
    // when destroy instance, flush last loggroup
    _push_last_loggroup(manager);

    aos_info_log("flush out producer loggroup begin");
    int32_t total_wait_count = manager->producer_config->destroyFlusherWaitTimeoutSec > 0 ? manager->producer_config->destroyFlusherWaitTimeoutSec * 100 : MAX_MANAGER_FLUSH_COUNT;
    total_wait_count += manager->producer_config->destroySenderWaitTimeoutSec > 0 ? manager->producer_config->destroySenderWaitTimeoutSec * 100 : MAX_SENDER_FLUSH_COUNT;

    usleep(10 * 1000);
    int waitCount = 0;
    while (log_queue_size(manager->loggroup_queue) > 0 ||
            manager->send_param_queue_write - manager->send_param_queue_read > 0 ||
            (manager->sender_data_queue != NULL && log_queue_size(manager->sender_data_queue) > 0) )
    {
        usleep(10 * 1000);
        if (++waitCount == total_wait_count)
        {
            break;
        }
    }
    if (waitCount == total_wait_count)
    {
        aos_error_log("try flush out producer loggroup error, force exit, now loggroup %d", (int)(log_queue_size(manager->loggroup_queue)));
    }
    else
    {
        aos_info_log("flush out producer loggroup success");
    }
    manager->shutdown = 1;

    // destroy root resources
    COND_SIGNAL(manager->triger_cond);
    aos_info_log("join flush thread begin");
    THREAD_JOIN(manager->flush_thread);
    aos_info_log("join flush thread success");
    if (manager->send_threads != NULL)
    {
        aos_info_log("join sender thread pool begin");
        int32_t threadId = 0;
        for (; threadId < manager->producer_config->sendThreadCount; ++threadId)
        {
            THREAD_JOIN(manager->send_threads[threadId]);
        }
        free(manager->send_threads);
        aos_info_log("join sender thread pool success");
    }
    DeleteCond(manager->triger_cond);
    log_queue_destroy(manager->loggroup_queue);
    if (manager->sender_data_queue != NULL)
    {
        aos_info_log("flush out sender queue begin");
        while (log_queue_size(manager->sender_data_queue) > 0)
        {
            void * send_param = log_queue_trypop(manager->sender_data_queue);
            if (send_param != NULL)
            {
                log_producer_send_fun(send_param);
            }
        }
        log_queue_destroy(manager->sender_data_queue);
        aos_info_log("flush out sender queue success");
    }
    else if (g_sender_data_queue != NULL && g_send_threads != NULL)
    {
        // if use global send queue, let send thread destroy manager
        log_producer_send_param * destroy_param = create_log_producer_destroy_param(manager->producer_config, manager);
        // make sure push success
        while(0 != log_queue_push(g_sender_data_queue, destroy_param))
        {
            ;
        }
        return;
    }

    destroy_log_producer_manager_tail(manager);

}

log_producer_result log_producer_manager_add_log(log_producer_manager * producer_manager, uint32_t log_time, int32_t pair_count, char ** keys, size_t * key_lens, char ** values, size_t * val_lens)
{
    if (producer_manager->totalBufferSize > producer_manager->producer_config->maxBufferBytes)
    {
        return LOG_PRODUCER_DROP_ERROR;
    }
    CS_ENTER(producer_manager->lock);
    if (producer_manager->builder == NULL)
    {
        // if queue is full, return drop error
        if (log_queue_isfull(producer_manager->loggroup_queue))
        {
            CS_LEAVE(producer_manager->lock);
            return LOG_PRODUCER_DROP_ERROR;
        }
        int32_t now_time = time(NULL);

        producer_manager->builder = log_group_create();
        producer_manager->firstLogTime = now_time;
        producer_manager->builder->private_value = producer_manager;
    }

    add_log_full(producer_manager->builder, log_time, pair_count, keys, key_lens, values, val_lens);

    log_group_builder * builder = producer_manager->builder;

    int32_t nowTime = time(NULL);
    if (producer_manager->builder->loggroup_size < producer_manager->producer_config->logBytesPerPackage &&
            nowTime - producer_manager->firstLogTime < producer_manager->producer_config->packageTimeoutInMS / 1000 &&
            producer_manager->builder->grp->n_logs < producer_manager->producer_config->logCountPerPackage)
    {
        CS_LEAVE(producer_manager->lock);
        return LOG_PRODUCER_OK;
    }

    producer_manager->builder = NULL;

    size_t loggroup_size = builder->loggroup_size;
    aos_debug_log("try push loggroup to flusher, size : %d, log count %d", (int)builder->loggroup_size, (int)builder->grp->n_logs);
    int status = log_queue_push(producer_manager->loggroup_queue, builder);
    if (status != 0)
    {
        aos_error_log("try push loggroup to flusher failed, force drop this log group, error code : %d", status);
        log_group_destroy(builder);
    }
    else
    {
        producer_manager->totalBufferSize += loggroup_size;
        COND_SIGNAL(producer_manager->triger_cond);
    }

    CS_LEAVE(producer_manager->lock);

    return LOG_PRODUCER_OK;
}

log_producer_result log_producer_manager_send_raw_buffer(log_producer_manager * producer_manager, size_t log_bytes, size_t compressed_bytes, const unsigned char * raw_buffer)
{
    // pack lz4_log_buf
    lz4_log_buf* lz4_buf = (lz4_log_buf*)malloc(sizeof(lz4_log_buf) + compressed_bytes);
    lz4_buf->length = compressed_bytes;
    lz4_buf->raw_length = log_bytes;
    memcpy(lz4_buf->data, raw_buffer, compressed_bytes);
    log_producer_send_param * send_param = create_log_producer_send_param(producer_manager->producer_config, producer_manager, lz4_buf, time(NULL));

    CS_ENTER(producer_manager->lock);
    producer_manager->totalBufferSize += lz4_buf->length;
    CS_LEAVE(producer_manager->lock);

    if (producer_manager->send_threads != NULL) {
        return log_queue_push(producer_manager->sender_data_queue, send_param) == 0 ? LOG_PRODUCER_OK : LOG_PRODUCER_DROP_ERROR;
    }
    return log_producer_send_data(send_param);
}



