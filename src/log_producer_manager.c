//
// Created by ZhangCheng on 20/11/2017.
//

#include "log_producer_manager.h"
#include "aos_log.h"
#include "apr_md5.h"

#define LOG_PRODUCER_FLUSH_INTERVAL_MS 100


#define MAX_LOGGROUP_QUEUE_SIZE 1024
#define MIN_LOGGROUP_QUEUE_SIZE 32

static int g_priority_map_array[] = {
        0,
        APR_THREAD_TASK_PRIORITY_LOWEST,
        APR_THREAD_TASK_PRIORITY_LOW,
        APR_THREAD_TASK_PRIORITY_NORMAL,
        APR_THREAD_TASK_PRIORITY_HIGH,
        APR_THREAD_TASK_PRIORITY_HIGHEST
};

char * _get_pack_id(const char * configName, const char * ip, apr_pool_t * pool)
{
    unsigned char md5Buf[17];
    apr_md5_ctx_t context;
    if (0 != apr_md5_init(&context)) {
        return NULL;
    }

    if (0 != apr_md5_update(&context, configName, strlen(configName))) {
        return NULL;
    }

    if (0 != apr_md5_update(&context, ip, strlen(ip))) {
        return NULL;
    }

    apr_time_t now = apr_time_now();
    if (0 != apr_md5_update(&context, &now, sizeof(apr_time_t))) {
        return NULL;
    }

    if (0 != apr_md5_final(md5Buf, &context)) {
        return NULL;
    }
    md5Buf[16] = '\0';

    char * result = (char *)apr_palloc(pool, 32);
    memset(result, 0, 32);

    int loop = 0;
    for(; loop < 8; ++loop)
    {
        unsigned char a = ((md5Buf[loop])>>4) & 0xF, b = (md5Buf[loop]) & 0xF;
        result[loop<<1] = a > 9 ? (a - 10 + 'A') : (a + '0');
        result[(loop<<1)|1] = b > 9 ? (b - 10 + 'A') : (b + '0');
    }
    return result;
}

void _try_flush_loggroup(log_producer_manager * producer_manager)
{
    apr_time_t now_time = apr_time_now();

    apr_thread_mutex_lock(producer_manager->lock);
    if (producer_manager->builder != NULL && now_time - producer_manager->firstLogTime > producer_manager->producer_config->packageTimeoutInMS * 1000)
    {
        log_group_builder * builder = producer_manager->builder;
        producer_manager->builder = NULL;
        apr_thread_mutex_unlock(producer_manager->lock);

        apr_status_t status = apr_queue_trypush(producer_manager->loggroup_queue, builder);
        aos_debug_log("try push loggroup to flusher, size : %d, status : %d", (int)builder->loggroup_size, status);
        if (status != LOG_PRODUCER_OK)
        {
            aos_error_log("try push loggroup to flusher failed, force drop this log group, error code : %d", status);
            log_group_destroy(builder);
        }
        else
        {
            apr_atomic_add32(&producer_manager->totalBufferSize, (apr_uint32_t)builder->loggroup_size);
            apr_thread_cond_signal(producer_manager->triger_cond);
        }
    }
    else
    {
        apr_thread_mutex_unlock(producer_manager->lock);
    }
}

int _log_producer_manager_flush_sub(void *rec, const char *key,
                                     const char *value)
{
    _try_flush_loggroup((log_producer_manager *)value);
    return 1;
}


void * log_producer_flush_thread(apr_thread_t * thread, void * param)
{
    log_producer_manager * root_producer_manager = (log_producer_manager*)param;
    aos_info_log("start run flusher thread, config : %s", root_producer_manager->producer_config->configName);
    while (root_producer_manager->shutdown == 0)
    {
        apr_thread_mutex_lock(root_producer_manager->triger_lock);
        apr_thread_cond_timedwait(root_producer_manager->triger_cond, root_producer_manager->triger_lock, (apr_interval_time_t)LOG_PRODUCER_FLUSH_INTERVAL_MS * 1000);
        apr_thread_mutex_unlock(root_producer_manager->triger_lock);



//        aos_debug_log("run flusher thread, config : %s, now loggroup size : %d, delta time : %d",
//                      producer_manager->producer_config->configName,
//                      producer_manager->builder != NULL ? (int)producer_manager->builder->loggroup_size : 0,
//                      (int)(now_time - producer_manager->firstLogTime));

        // try read queue
        do
        {
            void * data = NULL;
            apr_status_t read_status = apr_queue_trypop(root_producer_manager->loggroup_queue, &data);
            if (read_status == APR_SUCCESS)
            {
                // process data
                log_group_builder * builder = (log_group_builder*)data;

                log_producer_manager * producer_manager = (log_producer_manager *)builder->private_value;
                apr_atomic_sub32(&producer_manager->totalBufferSize, (apr_uint32_t)builder->loggroup_size);

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

                lz4_log_buf * lz4_buf = serialize_to_proto_buf_with_malloc_lz4(builder);

                if (lz4_buf == NULL)
                {
                    aos_error_log("serialize loggroup to proto buf with lz4 failed");
                }
                else
                {
                    aos_debug_log("push loggroup to sender, config %s, loggroup size %d, lz4 size %d, now buffer size %d",
                                  config->configName, (int)lz4_buf->raw_length, (int)lz4_buf->length, (int)producer_manager->totalBufferSize);
                    // if use multi thread, should change producer_manager->send_pool to NULL
                    //apr_pool_t * pool = config->sendThreadCount == 1 ? producer_manager->send_pool : NULL;
                    apr_pool_t * pool = NULL;
                    log_producer_send_param * send_param = create_log_producer_send_param(config, producer_manager, lz4_buf, pool, builder->builder_time);
                    log_producer_send_data(send_param);
                }

                log_group_destroy(builder);
                continue;
            }
            break;
        }while(1);

        // if no job, check now loggroup
        _try_flush_loggroup(root_producer_manager);

        if (root_producer_manager->sub_managers != NULL)
        {
            apr_table_do(_log_producer_manager_flush_sub, NULL, root_producer_manager->sub_managers, NULL);
        }

    }
    aos_info_log("exit flusher thread, config : %s", root_producer_manager->producer_config->configName);
    return NULL;
}

log_producer_manager * _create_log_producer_manager(apr_pool_t * root, log_producer_config * producer_config, log_producer_manager * parent_manager);

int _log_producer_manager_create_sub(void *rec, const char *key,
                                  const char *value)
{
    log_producer_manager * parent_manager = (log_producer_manager *)rec;
    log_producer_config * sub_config = (log_producer_config *)value;
    log_producer_manager * sub_manager = _create_log_producer_manager(parent_manager->root, sub_config, parent_manager);
    if (sub_manager != NULL)
    {
        apr_table_setn(parent_manager->sub_managers, key, (const char *)sub_manager);
    }
    return 1;
}


log_producer_manager * _create_log_producer_manager(apr_pool_t * root, log_producer_config * producer_config, log_producer_manager * parent_manager)
{
    aos_debug_log("create log producer manager : %s", producer_config->configName);
    log_producer_manager * producer_manager = (log_producer_manager *)apr_palloc(root, sizeof(log_producer_manager));
    memset(producer_manager, 0, sizeof(log_producer_manager));

    producer_manager->producer_config = producer_config;
    producer_manager->priority = g_priority_map_array[producer_config->priority];
    producer_manager->root = root;


    apr_status_t status = APR_SUCCESS;
    if (parent_manager == NULL)
    {
        apr_pool_create(&(producer_manager->send_pool), NULL);

        int32_t base_queue_size = producer_config->maxBufferBytes / (producer_config->logBytesPerPackage + 1) + 10;
        if (base_queue_size < MIN_LOGGROUP_QUEUE_SIZE)
        {
            base_queue_size = MIN_LOGGROUP_QUEUE_SIZE;
        }
        else if (base_queue_size > MAX_LOGGROUP_QUEUE_SIZE)
        {
            base_queue_size = MAX_LOGGROUP_QUEUE_SIZE;
        }

        status = apr_queue_create(&(producer_manager->loggroup_queue), base_queue_size, root);
        if (status != APR_SUCCESS)
        {
            aos_fatal_log("create log producer manager fail on create queue, error code : %d", status);
            goto create_fail;
        }

        status = apr_thread_cond_create(&(producer_manager->triger_cond), root);
        if (status != APR_SUCCESS)
        {
            aos_fatal_log("create log producer manager fail on init triger cond, error code : %d", status);
            goto create_fail;
        }

        status = apr_thread_mutex_create(&(producer_manager->triger_lock), APR_THREAD_MUTEX_DEFAULT, root);
        if (status != APR_SUCCESS)
        {
            aos_fatal_log("create log producer manager fail on create triger lock, error code : %d", status);
            goto create_fail;
        }

        producer_manager->sender = create_log_producer_sender(producer_config, root);
        if (producer_manager->sender == NULL)
        {
            aos_fatal_log("create log producer manager fail on create sender.");
            goto create_fail;
        }

        status = apr_thread_create(&(producer_manager->flush_thread), NULL, log_producer_flush_thread, producer_manager, root);
        if (status != APR_SUCCESS)
        {
            aos_fatal_log("create log producer manager fail on create flusher thread, error code : %d", status);
            goto create_fail;
        }
    }
    else
    {
        producer_manager->send_pool = parent_manager->send_pool;
        producer_manager->loggroup_queue = parent_manager->loggroup_queue;
        producer_manager->triger_cond = parent_manager->triger_cond;
        producer_manager->triger_lock = parent_manager->triger_lock;
        producer_manager->sender = parent_manager->sender;
        producer_manager->flush_thread = parent_manager->flush_thread;
    }

    if (producer_config->debugOpen)
    {
        producer_manager->debuger = create_log_producer_debug_flusher(producer_config);
    }

    status = apr_thread_mutex_create(&(producer_manager->lock), APR_THREAD_MUTEX_DEFAULT, root);
    if (status != APR_SUCCESS)
    {
        aos_fatal_log("create log producer manager fail on create lock, error code : %d", status);
        goto create_fail;
    }

    producer_manager->source = get_ip_by_host_name(producer_manager->root);
    if (producer_manager->source == NULL)
    {
        producer_manager->source = "unknown";
    }

    producer_manager->pack_prefix = _get_pack_id(producer_config->configName, producer_manager->source, producer_manager->root);
    if (producer_manager->pack_prefix == NULL)
    {
        producer_manager->pack_prefix = (char *)apr_palloc(producer_manager->root, 32);
        srand(time(NULL));
        int i = 0;
        for (i = 0; i < 16; ++i)
        {
            producer_manager->pack_prefix[i] = rand() % 10 + '0';
        }
        producer_manager->pack_prefix[i] = '\0';
    }

    if (producer_config->subConfigs != NULL)
    {
        producer_manager->sub_managers = apr_table_make(root, 4);
        apr_table_do(_log_producer_manager_create_sub, producer_manager, producer_config->subConfigs, NULL);
    }

    return producer_manager;

    create_fail:

    // todo, destroy queue, cont, sender...
    return NULL;

}

log_producer_manager * create_log_producer_manager(apr_pool_t * root, log_producer_config * producer_config)
{
    return _create_log_producer_manager(root, producer_config, NULL);
}

void _push_last_loggroup(log_producer_manager * manager)
{
    apr_thread_mutex_lock(manager->lock);
    log_group_builder * builder = manager->builder;
    manager->builder = NULL;
    apr_thread_mutex_unlock(manager->lock);
    if (builder != NULL)
    {
        apr_status_t status = apr_queue_trypush(manager->loggroup_queue, builder);
        aos_debug_log("try push loggroup to flusher, size : %d, log count %d, status : %d", (int)builder->loggroup_size, (int)builder->grp->n_logs, status);
        if (status != LOG_PRODUCER_OK)
        {
            aos_error_log("try push loggroup to flusher failed, force drop this log group, error code : %d", status);
            log_group_destroy(builder);
        }
        else
        {
            apr_atomic_add32(&manager->totalBufferSize, (apr_uint32_t)builder->loggroup_size);
            apr_thread_cond_signal(manager->triger_cond);
        }
    }
}

void _destroy_manager_single(log_producer_manager * manager)
{
    apr_thread_mutex_destroy(manager->lock);
    if (manager->debuger != NULL)
    {
        destroy_log_producer_debug_flusher(manager->debuger);
    }
}

int _log_producer_manager_push_last_sub(void *rec, const char *key,
                                     const char *value)
{
    log_producer_manager * manager = (log_producer_manager *)value;
    _push_last_loggroup(manager);
    return 1;
}

int _log_producer_manager_destroy_sub(void *rec, const char *key,
                                        const char *value)
{
    log_producer_manager * manager = (log_producer_manager *)value;
    _destroy_manager_single(manager);
    return 1;
}



void destroy_log_producer_manager(log_producer_manager * manager)
{
    // when destroy instance, flush last loggroup
    _push_last_loggroup(manager);

    if (manager->sub_managers != NULL)
    {
        apr_table_do(_log_producer_manager_push_last_sub, NULL, manager->sub_managers, NULL);
    }

    int waitCount = 0;
    while (apr_queue_size(manager->loggroup_queue) > 0)
    {
        usleep(10 * 1000);
        if (++waitCount == 100)
        {
            break;
        }
    }
    if (waitCount == 100)
    {
        aos_error_log("try flush out producer loggroup error, force exit, now loggroup %d", (int)(apr_queue_size(manager->loggroup_queue)));
    }
    manager->shutdown = 1;

    // destroy root resources
    apr_thread_cond_signal(manager->triger_cond);
    apr_queue_interrupt_all(manager->loggroup_queue);
    apr_status_t ret_val = APR_SUCCESS;
    apr_thread_join(&ret_val, manager->flush_thread);
    destroy_log_producer_sender(manager->sender);
    apr_thread_cond_destroy(manager->triger_cond);
    apr_thread_mutex_destroy(manager->triger_lock);
    apr_queue_term(manager->loggroup_queue);
    apr_pool_destroy(manager->send_pool);


    _destroy_manager_single(manager);

    if (manager->sub_managers != NULL)
    {
        apr_table_do(_log_producer_manager_destroy_sub, NULL, manager->sub_managers, NULL);
    }
}

log_producer_result log_producer_manager_add_log_start(log_producer_manager * producer_manager)
{
    if (apr_atomic_read32(&(producer_manager->totalBufferSize)) > (uint32_t)producer_manager->producer_config->maxBufferBytes)
    {
        return LOG_PRODUCER_DROP_ERROR;
    }
    apr_thread_mutex_lock(producer_manager->lock);
    if (producer_manager->builder == NULL)
    {
        apr_time_t now_time = apr_time_now();

        producer_manager->builder = log_group_create();
        producer_manager->firstLogTime = now_time;
        producer_manager->builder->private_value = producer_manager;
    }

    add_log(producer_manager->builder);
    add_log_time(producer_manager->builder, (u_int32_t)time(NULL));
    return LOG_PRODUCER_OK;
}

log_producer_result log_producer_manager_add_log_end(log_producer_manager * producer_manager)
{

    log_group_builder * builder = producer_manager->builder;

    if (producer_manager->debuger != NULL)
    {
        log_producer_debug_flusher_write(producer_manager->debuger, builder->lg);
    }
    apr_time_t nowTime = apr_time_now();
    if (producer_manager->builder->loggroup_size < producer_manager->producer_config->logBytesPerPackage &&
            nowTime - producer_manager->firstLogTime < producer_manager->producer_config->packageTimeoutInMS * 1000 &&
            producer_manager->builder->grp->n_logs < producer_manager->producer_config->logCountPerPackage)
    {
        apr_thread_mutex_unlock(producer_manager->lock);
        return LOG_PRODUCER_OK;
    }

    producer_manager->builder = NULL;

    apr_thread_mutex_unlock(producer_manager->lock);


    apr_status_t status = apr_queue_trypush(producer_manager->loggroup_queue, builder);
    aos_debug_log("try push loggroup to flusher, size : %d, log count %d, status : %d", (int)builder->loggroup_size, (int)builder->grp->n_logs, status);
    if (status != LOG_PRODUCER_OK)
    {
        aos_error_log("try push loggroup to flusher failed, force drop this log group, error code : %d", status);
        log_group_destroy(builder);
    }
    else
    {
        apr_atomic_add32(&producer_manager->totalBufferSize, (apr_uint32_t)builder->loggroup_size);
        apr_thread_cond_signal(producer_manager->triger_cond);
    }

    return LOG_PRODUCER_OK;
}

log_producer_result log_producer_manager_add_log_kv(log_producer_manager * producer_manager, const char * key, const char * value)
{
    add_log_key_value(producer_manager->builder, key, strlen(key), value, strlen(value));
    return LOG_PRODUCER_OK;
}

log_producer_result log_producer_manager_add_log_kv_len(log_producer_manager * producer_manager, const char * key, size_t key_len,  const char * value, size_t value_len)
{
    add_log_key_value(producer_manager->builder, key, key_len, value, value_len);
    return LOG_PRODUCER_OK;
}

