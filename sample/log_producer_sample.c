//
// Created by ZhangCheng on 24/11/2017.
//

#include "inner_log.h"
#include "log_api.h"
#include "log_producer_config.h"
#include "log_producer_client.h"
#include <pthread.h>
#include <unistd.h>
#include <string.h>


void builder_speed_test(int32_t logsPerGroup)
{
    int32_t startTime = time(NULL);
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
            char * keys[] = {
                    "content_key_1",
                    "content_key_1",
                    "content_key_1",
                    "content_key_1",
                    "content_key_1",
                    "content_key_1",
                    "content_key_1",
                    "content_key_1",
                    "content_key_1",
                    "content_key_1"
            };
            char * vals[] = {
                    "1abcdefghijklmnopqrstuvwxyz01234567891abcdefghijklmnopqrstuvwxyz01234567891abcdefghijklmnopqrstuvwxyz01234567891abcdefghijklmnopqrstuvwxyz0123456789",
                    "2abcdefghijklmnopqrstuvwxyz0123456789",
                    "2abcdefghijklmnopqrstuvwxyz0123456789",
                    "2abcdefghijklmnopqrstuvwxyz0123456789",
                    "2abcdefghijklmnopqrstuvwxyz0123456789",
                    "2abcdefghijklmnopqrstuvwxyz0123456789",
                    "2abcdefghijklmnopqrstuvwxyz0123456789",
                    "2abcdefghijklmnopqrstuvwxyz0123456789",
                    "2abcdefghijklmnopqrstuvwxyz0123456789",
                    "xxxxxxxxxxxxxxxxxxxxx"
            };
            size_t key_lens[] = {
                    strlen("content_key_1"),
                    strlen("content_key_1"),
                    strlen("content_key_1"),
                    strlen("content_key_1"),
                    strlen("content_key_1"),
                    strlen("content_key_1"),
                    strlen("content_key_1"),
                    strlen("content_key_1"),
                    strlen("content_key_1"),
                    strlen("index")
            };
            size_t val_lens[] = {
                    strlen("1abcdefghijklmnopqrstuvwxyz01234567891abcdefghijklmnopqrstuvwxyz01234567891abcdefghijklmnopqrstuvwxyz01234567891abcdefghijklmnopqrstuvwxyz0123456789"),
                    strlen("2abcdefghijklmnopqrstuvwxyz0123456789"),
                    strlen("2abcdefghijklmnopqrstuvwxyz0123456789"),
                    strlen("2abcdefghijklmnopqrstuvwxyz0123456789"),
                    strlen("2abcdefghijklmnopqrstuvwxyz0123456789"),
                    strlen("2abcdefghijklmnopqrstuvwxyz0123456789"),
                    strlen("2abcdefghijklmnopqrstuvwxyz0123456789"),
                    strlen("2abcdefghijklmnopqrstuvwxyz0123456789"),
                    strlen("2abcdefghijklmnopqrstuvwxyz0123456789"),
                    strlen("xxxxxxxxxxxxxxxxxxxxx")
            };
            add_log_full(bder, (uint32_t)time(NULL), 10, (char **)keys, (size_t *)key_lens, (char **)vals, (size_t *)val_lens);
        }
        log_buf buf = serialize_to_proto_buf_with_malloc(bder);

        if (count % 1000 == 0)
        {
            int32_t nowTime = time(NULL);
            aos_error_log("Done : %d %d %d %d\n", count, logsPerGroup, nowTime - startTime, (int32_t)buf.n_buffer);
        }
        log_group_destroy(bder);
    }
    aos_error_log("total time sec  %d ", (time(NULL) - startTime));
}

void on_log_send_done(const char * config_name, log_producer_result result, size_t log_bytes, size_t compressed_bytes, const char * req_id, const char * message)
{
    if (result == LOG_PRODUCER_OK)
    {
        printf("send success, config : %s, result : %d, log bytes : %d, compressed bytes : %d, request id : %s \n",
               config_name, (result),
               (int)log_bytes, (int)compressed_bytes, req_id);
    }
    else
    {
        printf("send fail, config : %s, result : %d, log bytes : %d, compressed bytes : %d, request id : %s, error message : %s\n",
               config_name, (result),
               (int)log_bytes, (int)compressed_bytes, req_id, message);
    }

}

typedef struct _multi_write_log_param
{
    log_producer_client * client;
    int32_t send_count;
}multi_write_log_param;

void * write_log_thread(void* param)
{
    aos_error_log("Thread start");
    multi_write_log_param * write_log_param = (multi_write_log_param *)param;
    int32_t i = 0;
    for (; i < write_log_param->send_count; ++i)
    {
        log_producer_client_add_log(write_log_param->client, 8, "Log group", "A log group is a collection of logs and is the basic unit for writing and reading",
                          "Log topic", "Logs in a Logstore can be classified by log topics",
                          "Project", "The project is the resource management unit in Log Service and is used to isolate and control resources",
                          "Logstore", "The Logstore is a unit in Log Service for the collection, storage, and query of log data");
    }
    aos_error_log("Thread done");
    return NULL;
}

log_producer * create_log_producer_wrapper(on_log_producer_send_done_function on_send_done)
{
    log_producer_config * config = create_log_producer_config();
    log_producer_config_set_endpoint(config, "http://{your_endpoint}");
    log_producer_config_set_project(config, "{your_project}");
    log_producer_config_set_logstore(config, "{your_logstore}");
    log_producer_config_set_access_id(config, "{your_access_key}");
    log_producer_config_set_access_key(config, "{your_access_key_secret}");

    // if you do not need topic or tag, comment it
    log_producer_config_set_topic(config, "test_topic");
    log_producer_config_add_tag(config, "tag_1", "val_1");
    log_producer_config_add_tag(config, "tag_2", "val_2");
    log_producer_config_add_tag(config, "tag_3", "val_3");
    log_producer_config_add_tag(config, "tag_4", "val_4");
    log_producer_config_add_tag(config, "tag_5", "val_5");

    // set resource params
    log_producer_config_set_packet_log_bytes(config, 1024*1024*3);
    log_producer_config_set_packet_log_count(config, 2048);
    log_producer_config_set_packet_timeout(config, 3000);
    log_producer_config_set_max_buffer_limit(config, 64*1024*1024);


    return create_log_producer(config, on_send_done);
}

#define MUTLI_THREAD_COUNT 16

void log_producer_multi_thread(int logsPerSecond)
{
    logsPerSecond *= 100;
    if (log_producer_env_init() != LOG_PRODUCER_OK) {
        exit(1);
    }

    log_producer * producer = create_log_producer_wrapper(on_log_send_done);
    if (producer == NULL)
    {
        printf("create log producer by config file fail \n");
        exit(1);
    }

    log_producer_client * client = get_log_producer_client(producer, NULL);
    if (client == NULL)
    {
        printf("create log producer client by config file fail \n");
        exit(1);
    }

    log_producer_client * client2 = get_log_producer_client(producer, "test_sub_config");
    if (client2 == NULL)
    {
        printf("create log producer client by config file fail \n");
        exit(1);
    }
    //assert(client != client2);


    multi_write_log_param param[2];
    param[0].send_count = logsPerSecond;
    param[0].client = client;


    param[1].send_count = logsPerSecond;
    param[1].client = client2;

    pthread_t allThread[MUTLI_THREAD_COUNT];
    int i = 0;
    for (i = 0; i < MUTLI_THREAD_COUNT; ++i)
    {
        pthread_create(&allThread[i], NULL, write_log_thread, &param[i % 2]);
    }


    for (i = 0; i < MUTLI_THREAD_COUNT; ++i)
    {
        pthread_join(allThread[i], NULL);
    }
    aos_error_log("All thread done");

    destroy_log_producer(producer);

    log_producer_env_destroy();

}

void log_producer_create_destroy()
{
    int count = 0;
    while (1)
    {
        if (log_producer_env_init() != LOG_PRODUCER_OK) {
            exit(1);
        }

        log_producer * producer = create_log_producer_wrapper(on_log_send_done);
        if (producer == NULL)
        {
            printf("create log producer by config file fail \n");
            exit(1);
        }

        log_producer_client * client = get_log_producer_client(producer, NULL);
        if (client == NULL)
        {
            printf("create log producer client by config file fail \n");
            exit(1);
        }

        destroy_log_producer(producer);

        log_producer_env_destroy();
        printf("%d \n", count++);
    }
}

void log_producer_post_logs(int logsPerSecond, int sendSec)
{
    aos_log_level = AOS_LOG_DEBUG;
    if (log_producer_env_init() != LOG_PRODUCER_OK) {
        exit(1);
    }

    log_producer * producer = create_log_producer_wrapper(on_log_send_done);
    if (producer == NULL)
    {
        printf("create log producer by config file fail \n");
        exit(1);
    }

    log_producer_client * client = get_log_producer_client(producer, NULL);
    if (client == NULL)
    {
        printf("create log producer client by config file fail \n");
        exit(1);
    }


    int32_t i = 0;
    int32_t totalTime = 0;
    for (i = 0; i < sendSec; ++i)
    {
        int32_t startTime = time(NULL);
        int j = 0;
        for (; j < logsPerSecond; ++j)
        {
            char indexStr[32];
            sprintf(indexStr, "%d", i * logsPerSecond + j);
            log_producer_client_add_log(client, 20, "content_key_1", "1abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+",
                                        "content_key_2", "2abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+",
                                        "content_key_3", "3abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+",
                                        "content_key_4", "4abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+",
                                        "content_key_5", "5abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+",
                                        "content_key_6", "6abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+",
                                        "content_key_7", "7abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+",
                                        "content_key_8", "8abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+",
                                        "content_key_9", "9abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+",
                                        "index", indexStr);

            log_producer_client_add_log(client, 8, "LogHub", "Real-time log collection and consumption",
                              "Search/Analytics", "Query and real-time analysis",
                              "Visualized", "dashboard and report functions",
                              "Interconnection", "Grafana and JDBC/SQL92");
        }
        int32_t endTime = time(NULL);
        aos_error_log("Done : %d  %d time  %f us \n", i, logsPerSecond, (float)(endTime - startTime));
        totalTime += endTime - startTime;
        if (endTime - startTime < 1)
        {
            usleep(1000000);
        }
    }
    aos_error_log("Total done : %f us, avg %f us", (float)totalTime / 180, (float)totalTime / (180 * logsPerSecond * 2));
    //sleep(10);

    destroy_log_producer(producer);

    log_producer_env_destroy();
}

int main(int argc, char *argv[])
{
    //aos_log_level = AOS_LOG_TRACE;
    int logsPerSec = 100;
    int sendSec = 180;
    if (argc == 3)
    {
        logsPerSec = atoi(argv[1]);
        sendSec = atoi(argv[2]);
    }
    //log_producer_create_destroy();
    //log_producer_multi_thread(logsPerSec / 10);
    log_producer_post_logs(logsPerSec, sendSec);
    //post_logs_to_debuger(filePath);
    //builder_speed_test(logsPerSec);
    return 0;
}

