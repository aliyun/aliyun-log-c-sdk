#include "log_api.h"
#include "log_producer_config.h"
#include "log_producer_client.h"

void on_log_send_done(const char * config_name, log_producer_result result, size_t log_bytes, size_t compressed_bytes, const char * req_id, const char * message, const unsigned char * raw_buffer)
{
    if (result == LOG_PRODUCER_OK)
    {
        if (req_id == NULL)
        {
            req_id = "";
        }
        printf("send success, config : %s, result : %d, log bytes : %d, compressed bytes : %d, request id : %s\n",
               config_name, (result),
               (int)log_bytes, (int)compressed_bytes, req_id);
    }
    else
    {
        if (message == NULL)
        {
            message = "";
        }
        if (req_id == NULL)
        {
            req_id = "";
        }
        printf("send fail, config : %s, result : %d, log bytes : %d, compressed bytes : %d, request id : %s, error message : %s\n",
               config_name, (result),
               (int)log_bytes, (int)compressed_bytes, req_id, message);
    }
}

void log_producer_post_logs()
{
    if (log_producer_env_init(LOG_GLOBAL_ALL) != LOG_PRODUCER_OK) {
        exit(1);
    }

    log_producer_config * config = create_log_producer_config();

    // endpoint must start with "https://" for API-Key mode
    // endpoint list: https://help.aliyun.com/document_detail/29008.html
    log_producer_config_set_endpoint(config, "https://your-endpoint.log.aliyuncs.com");
    log_producer_config_set_project(config, "your-project");
    log_producer_config_set_logstore(config, "your-logstore");

    // set API-Key (this automatically sets authVersion to AUTH_VERSION_APIKEY)
    // API-Key mode is mutually exclusive with AccessKey mode
    // do NOT call log_producer_config_set_access_id / set_access_key when using API-Key
    log_producer_config_set_api_key(config, "your-api-key");

    // optional: set topic and tags
    log_producer_config_set_topic(config, "test_topic");
    log_producer_config_add_tag(config, "tag_1", "val_1");

    // set resource params
    log_producer_config_set_packet_log_bytes(config, 4*1024*1024);
    log_producer_config_set_packet_log_count(config, 4096);
    log_producer_config_set_packet_timeout(config, 3000);
    log_producer_config_set_max_buffer_limit(config, 64*1024*1024);

    // set send thread count
    log_producer_config_set_send_thread_count(config, 4);

    log_producer * producer = create_log_producer(config, on_log_send_done);
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

    int i = 0;
    for (; i < 10; ++i)
    {
        char indexStr[32];
        sprintf(indexStr, "%d", i);
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

        log_producer_result rst = log_producer_client_add_log(client, 8, "LogHub", "Real-time log collection and consumption",
                          "Search/Analytics", "Query and real-time analysis",
                          "Visualized", "dashboard and report functions",
                          "Interconnection", "Grafana and JDBC/SQL92");
        if (rst != LOG_PRODUCER_OK)
        {
            printf("add log error %d \n", rst);
        }
    }

    destroy_log_producer(producer);

    log_producer_env_destroy();
}

int main(int argc, char *argv[])
{
    log_producer_post_logs();
    return 0;
}
