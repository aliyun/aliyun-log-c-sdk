//
// Created by ZhangCheng on 24/11/2017.
//

#include "aos_log.h"
#include "log_auth.h"
#include "log_api.h"
#include "log_producer_config.h"
#include "log_producer_debug_flusher.h"
#include "log_producer_client.h"
#include "apr_thread_proc.h"

log_post_option g_log_option;

void get_log_producer_config(const char * fileName)
{
    apr_initialize();
    log_producer_config * producerConfig = load_log_producer_config_file(fileName);
    if (producerConfig == NULL)
    {
        printf("load producer config fail \n");
        apr_terminate();
        return;
    }
    log_producer_config_print(producerConfig, stdout);
    destroy_log_producer_config(producerConfig);
    apr_terminate();
}

void post_logs_to_debuger(const char * fileName)
{
    apr_initialize();

    log_producer_config * producerConfig = load_log_producer_config_file(fileName);
    if (producerConfig == NULL)
    {
        printf("load producer config fail \n");
        apr_terminate();
        return;
    }

    log_producer_debug_flusher * debug_flusher = create_log_producer_debug_flusher(producerConfig);
    if (debug_flusher == NULL)
    {
        printf("create debug flusher fail \n");
        apr_terminate();
        return;
    }

    log_group_builder* bder = log_group_create();
    add_source(bder,"mSource",sizeof("mSource"));
    add_topic(bder,"mTopic", sizeof("mTopic"));

    add_tag(bder, "taga_key", strlen("taga_key"), "taga_value", strlen("taga_value"));
    add_tag(bder, "tagb_key", strlen("tagb_key"), "tagb_value", strlen("tagb_value"));
    add_pack_id(bder, "123456789ABC",  strlen("123456789ABC"), 0);

    int i;
    for(i=0;i<3;i++){
        add_log(bder);
        int j;
        for(j=0;j<5;j++){
            char k[] = {i*j + 'a'};
            char v[] = {2*j + 'a'} ;
            add_log_key_value(bder, k, 1, v, 1);
        }
    }



    for (i = 0; i < 1000000; ++i)
    {
        int j = 0;
        for (; j < 3; ++j)
        {
            log_lg * log = bder->grp->logs[j];
            log_producer_debug_flusher_write(debug_flusher, log);
            log_producer_print_log(log, stdout);
        }
    }



    log_group_destroy(bder);

    destroy_log_producer_debug_flusher(debug_flusher);
    destroy_log_producer_config(producerConfig);

    apr_terminate();
}

void builder_speed_test(int32_t logsPerGroup)
{
    apr_initialize();

    int32_t count = 0;
    for (; count < 1000000; ++count)
    {
        log_group_builder* bder = log_group_create();
        add_source(bder,"mSource",sizeof("mSource"));
        add_topic(bder,"mTopic", sizeof("mTopic"));

        add_tag(bder, "taga_key", strlen("taga_key"), "taga_value", strlen("taga_value"));
        add_tag(bder, "tagb_key", strlen("tagb_key"), "tagb_value", strlen("tagb_value"));
        add_pack_id(bder, "123456789ABC",  strlen("123456789ABC"), 0);

        int i = 0;
        for (; i < logsPerGroup; ++i)
        {
            add_log(bder);
            add_log_key_value(bder, "content_key_1", strlen("content_key_1"), "1abcdefghijklmnopqrstuvwxyz0123456789", strlen("1abcdefghijklmnopqrstuvwxyz0123456789"));
            add_log_key_value(bder, "content_key_1", strlen("content_key_2"), "2abcdefghijklmnopqrstuvwxyz0123456789", strlen("2abcdefghijklmnopqrstuvwxyz0123456789"));
            add_log_key_value(bder, "content_key_1", strlen("content_key_3"), "3abcdefghijklmnopqrstuvwxyz0123456789", strlen("3abcdefghijklmnopqrstuvwxyz0123456789"));
            add_log_key_value(bder, "content_key_1", strlen("content_key_4"), "4abcdefghijklmnopqrstuvwxyz0123456789", strlen("4abcdefghijklmnopqrstuvwxyz0123456789"));
            add_log_key_value(bder, "content_key_1", strlen("content_key_5"), "5abcdefghijklmnopqrstuvwxyz0123456789", strlen("5abcdefghijklmnopqrstuvwxyz0123456789"));
            add_log_key_value(bder, "content_key_1", strlen("content_key_6"), "6abcdefghijklmnopqrstuvwxyz0123456789", strlen("6abcdefghijklmnopqrstuvwxyz0123456789"));
            add_log_key_value(bder, "content_key_1", strlen("content_key_7"), "7abcdefghijklmnopqrstuvwxyz0123456789", strlen("7abcdefghijklmnopqrstuvwxyz0123456789"));
            add_log_key_value(bder, "content_key_1", strlen("content_key_8"), "8abcdefghijklmnopqrstuvwxyz0123456789", strlen("8abcdefghijklmnopqrstuvwxyz0123456789"));
            add_log_key_value(bder, "content_key_1", strlen("content_key_9"), "9abcdefghijklmnopqrstuvwxyz0123456789", strlen("9abcdefghijklmnopqrstuvwxyz0123456789"));
            add_log_key_value(bder, "content_key_1", strlen("index"), "xxxxxxxxxxxxxxxxxxxxx", strlen("xxxxxxxxxxxxxxxxxxxxx"));
        }
        lz4_log_buf * buf = serialize_to_proto_buf_with_malloc_lz4(bder);
        free(buf);
        log_group_destroy(bder);
        if (count % 1000 == 0)
        {
            aos_error_log("Done : %d %d\n", count, logsPerGroup);
        }
    }



    apr_terminate();
}

void on_log_send_done(log_producer_result result, size_t log_bytes, size_t compressed_bytes, const char * req_id, const char * message)
{
    if (result == LOG_PRODUCER_OK)
    {
        return;
    }
    printf("send fail, result : %s, log bytes : %d, compressed bytes : %d, request id : %s, error message : %s\n",
           get_log_producer_result_string(result),
           (int)log_bytes, (int)compressed_bytes, req_id, message);
}

typedef struct _multi_write_log_param
{
    log_producer_client * client;
    int32_t send_count;
}multi_write_log_param;

void * write_log_thread(apr_thread_t* thread_id, void* param)
{
    aos_error_log("Thread start");
    multi_write_log_param * write_log_param = (multi_write_log_param *)param;
    int32_t i = 0;
    for (; i < write_log_param->send_count; ++i)
    {
        LOG_PRODUCER_WARN(write_log_param->client, "Log group", "A log group is a collection of logs and is the basic unit for writing and reading",
                          "Log topic", "Logs in a Logstore can be classified by log topics",
                          "Project", "The project is the resource management unit in Log Service and is used to isolate and control resources",
                          "Logstore", "The Logstore is a unit in Log Service for the collection, storage, and query of log data");
    }
    aos_error_log("Thread done");
    return NULL;
}

#define MUTLI_THREAD_COUNT 16

void log_producer_multi_thread(const char * fileName, int logsPerSecond)
{
    logsPerSecond *= 100;
    if (log_producer_env_init() != LOG_PRODUCER_OK) {
        exit(1);
    }

    log_producer_client * client = create_log_producer_client_by_config_file(fileName, on_log_send_done);
    if (client == NULL)
    {
        printf("create log producer client by config file fail \n");
        exit(1);
    }
    multi_write_log_param param;
    param.send_count = logsPerSecond;
    param.client = client;

    apr_thread_t * allThread[MUTLI_THREAD_COUNT];
    apr_pool_t * root;
    apr_pool_create(&root, NULL);
    int i = 0;
    for (i = 0; i < MUTLI_THREAD_COUNT; ++i)
    {
        apr_thread_create(allThread + i, NULL, write_log_thread, &param, root);
    }


    for (i = 0; i < MUTLI_THREAD_COUNT; ++i)
    {
        apr_status_t result;
        apr_thread_join(&result, allThread[i]);
    }
    aos_error_log("All thread done");
    apr_pool_destroy(root);

    destroy_log_producer_client(client);

    log_producer_env_destroy();

}

void log_producer_post_logs(const char * fileName, int logsPerSecond)
{
    //aos_log_level = AOS_LOG_DEBUG;
    if (log_producer_env_init() != LOG_PRODUCER_OK) {
        exit(1);
    }

    log_producer_client * client = create_log_producer_client_by_config_file(fileName, on_log_send_done);
    if (client == NULL)
    {
        printf("create log producer client by config file fail \n");
        exit(1);
    }

    int32_t i = 0;
    apr_time_t totalTime = 0;
    for (i = 0; i < 180; ++i)
    {
        apr_time_t startTime = apr_time_now();
        int j = 0;
        for (; j < logsPerSecond; ++j)
        { 
        	log_producer_client_add_log(client, 20, "content_key_1", "1abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+",
                                    "content_key_2", "2abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+",
                                    "content_key_3", "3abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+",
                                    "content_key_4", "4abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+",
                                    "content_key_5", "5abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+",
                                    "content_key_6", "6abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+",
                                    "content_key_7", "7abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+",
                                    "content_key_8", "8abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+",
                                    "content_key_9", "9abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+",
                                    "index", "xxxxxxxxxxxxxxxxxxxxx");

        	LOG_PRODUCER_WARN(client, "LogHub", "Real-time log collection and consumption",
                              "Search/Analytics", "Query and real-time analysis",
                              "Visualized", "dashboard and report functions",
                              "Interconnection", "Grafana and JDBC/SQL92");
        	LOG_PRODUCER_DEBUG(client, "a", "v", "c", "a", "v");
        }
        apr_time_t endTime = apr_time_now(); 
        aos_error_log("Done : %d  %d time  %f us \n", i, logsPerSecond, (float)(endTime - startTime));
        totalTime += endTime - startTime;
        if (endTime - startTime < 1000000)
        {
            usleep(1000000 - endTime+startTime); 
        } 
    }
    aos_error_log("Total done : %f us, avg %f us", (float)totalTime / 180, (float)totalTime / (180 * logsPerSecond * 2));
    //sleep(10);

    destroy_log_producer_client(client);

    log_producer_env_destroy();
}

int main(int argc, char *argv[])
{
    const char * filePath = "./log_config.json";
    int logsPerSec = 100;
    if (argc == 3)
    {
        filePath = argv[1];
        logsPerSec = atoi(argv[2]);
    }
    log_producer_multi_thread(filePath, logsPerSec);
    //return 0;
    log_producer_post_logs(filePath, logsPerSec);
    post_logs_to_debuger(filePath);
    //builder_speed_test(logsPerSec);

    get_log_producer_config(filePath);
    return 0;
}
