//
// Created by ZhangCheng on 26/12/2017.
//

#include "log_http_interface.h"
#include "log_api.h"
#include "log_producer_config.h"
#include "log_producer_client.h"
#include "inner_log.h"
#include "log_util.h"


int returnCode = 200;

int mocHttpPost(const char *url,
char **header_array,
int header_count,
const void *data,
int data_len) {
    for (int i = 0; i < header_count; ++i) {
        printf("header: %s\n", header_array[i]);
    }
    return returnCode;
}

void on_log_send_done(const char * config_name,
                      log_producer_result result,
                      size_t log_bytes,
                      size_t compressed_bytes,
                      const char * req_id,
                      const char * message,
                      const unsigned char * raw_buffer,
                      void * userparams)
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

void sample_log_set_http_inject_header(log_producer_config *config, char **src_headers, int src_count, char **dest_headers, int *dest_count)
{
    for (int i = 0; i < src_count; ++i) {
        dest_headers[i] = src_headers[i];
    }

    dest_headers[src_count] = "User-agent: sls-android/test;";
    dest_headers[src_count + 1] = "testHeader: test";
    *dest_count = src_count + 2;


    for (int i = 0; i < src_count; ++i) {
        printf("src_header: %s \n", src_headers[i]);
    }

    for (int i = 0; i < *dest_count; ++i) {
        printf("dest_header: %s \n", dest_headers[i]);
    }
}

void sample_log_set_http_release_inject_header(log_producer_config *config, char **dest_headers, int dest_count)
{

}

log_producer * create_log_producer_wrapper(on_log_producer_send_done_function on_send_done)
{
    log_producer_config * config = create_log_producer_config();
    // endpoint list:  https://help.aliyun.com/document_detail/29008.html
    aos_log_set_level(AOS_LOG_ALL);
    log_producer_config_set_endpoint(config, "${your_endpoint}");
    log_producer_config_set_project(config, "${your_project}");
    log_producer_config_set_logstore(config, "${your_logstore}");
    log_producer_config_set_access_id(config, "${your_access_key_id}");
    log_producer_config_set_access_key(config, "${your_access_key_secret}");
    log_producer_config_set_use_webtracking(config, 0);


    // if you do not need topic or tag, comment it
    log_producer_config_set_topic(config, "test_topic");
    log_producer_config_add_tag(config, "tag_1", "val_1");
    log_producer_config_add_tag(config, "tag_2", "val_2");
    log_producer_config_add_tag(config, "tag_3", "val_3");
    log_producer_config_add_tag(config, "tag_4", "val_4");
    log_producer_config_add_tag(config, "tag_5", "val_5");

    // set resource params
    log_producer_config_set_packet_log_bytes(config, 512*1024);
    log_producer_config_set_packet_log_count(config, 4096);
    log_producer_config_set_packet_timeout(config, 3000);
    log_producer_config_set_max_buffer_limit(config, 4*1024*1024);
    log_producer_config_set_flush_interval(config, 10000);
    log_producer_config_set_log_queue_interval(config, 10000);
    log_set_http_header_inject_func(sample_log_set_http_inject_header);
    log_set_http_header_release_inject_func(sample_log_set_http_release_inject_header);

    // @note only 1 thread count, persistent client only support 1 thread
    log_producer_config_set_send_thread_count(config, 1);

    // set persistent
    log_producer_config_set_persistent(config, 1);
    log_producer_config_set_persistent_file_path(config, "/Users/gordon/Downloads/data/log.dat");
    log_producer_config_set_persistent_force_flush(config, 1);
    log_producer_config_set_persistent_max_file_count(config, 10);
    log_producer_config_set_persistent_max_file_size(config, 1024*1024);
    log_producer_config_set_persistent_max_log_count(config, 65536);

    log_set_http_post_func(mocHttpPost);

    return create_log_producer(config, on_send_done, NULL);
}


void log_producer_post_logs()
{
    if (log_producer_env_init() != LOG_PRODUCER_OK) {
        exit(1);
    }

    log_producer * producer = create_log_producer_wrapper(on_log_send_done);
    if (producer == NULL)
    {
        printf("create log producer by config fail \n");
        exit(1);
    }

    log_producer_client * client = get_log_producer_client(producer, NULL);
    if (client == NULL)
    {
        printf("create log producer client by config fail \n");
        exit(1);
    }
    returnCode = 200;

    //sleep(30000000);
    int i = 0;
    for (; i < 1; ++i)
    {
        char indexStr[32];
        sprintf(indexStr, "%d", i);
        while (1)
        {
            log_producer_result rst = log_producer_client_add_log(client, 20, "content_key_1", "1abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+",
                                        "content_key_2", "2abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+",
                                        "content_key_3", "3abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+",
                                        "content_key_4", "4abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+",
                                        "content_key_5", "5abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+",
                                        "content_key_6", "6abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+",
                                        "content_key_7", "7abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+",
                                        "content_key_8", "davidzhangggggggg",
                                        "content_key_9", "中文测试",
                                        "index", indexStr);

            //char * data = "1abcdefghijklmnopqrstuvwxyz0123456789";
            //uint32_t dataLen[4] = {1, 2, 3, 4};

            //rst = log_producer_client_add_log_with_array(client, 1263563523 + 1000, 2, data, dataLen, 0);

            if (rst != LOG_PRODUCER_OK)
            {
                printf("add log error %d \n", rst);
                log_sleep_ms(100);
                continue;
            }
            else
            {
                break;
            }
        }

        log_sleep_ms(1);
    }
    log_sleep_ms(1000);
    //_exit(0);




    destroy_log_producer(producer);

    log_producer_env_destroy();
}

int main(int argc, char *argv[])
{
    aos_log_set_level(AOS_LOG_INFO);
    log_producer_post_logs();
    return 0;
}

