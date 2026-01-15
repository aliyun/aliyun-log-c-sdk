#include "log_api.h"
#include "log_producer_config.h"
#include "log_producer_client.h"

void on_log_send_done(const char * config_name, log_producer_result result, size_t log_bytes, size_t compressed_bytes, const char * req_id, const char * message, const unsigned char * raw_buffer)
{
    if (result == LOG_PRODUCER_OK)
    {
        printf("send success\n");
    }
    else
    {
       printf("send fail\n");
    }
}

// user defined function to get credentials
int get_credentials_callback(log_producer_credentials * credentials, void * userdata)
{
    printf("get credentials callback\n");
    // this is just a example, you should get credentials from your own service
    const char* ak_id = "";
    const char* ak_secret = "";
    const char* token = ""; 
    int64_t expire_ts = time(NULL) + 1800; // replace with your own expire time

    return log_producer_credentials_set(
        credentials,
        ak_id, strlen(ak_id),
        ak_secret, strlen(ak_secret),
        token, strlen(token),
        expire_ts
    );
}

void log_producer_post_logs()
{
    if (log_producer_env_init(LOG_GLOBAL_ALL) != LOG_PRODUCER_OK) {
        exit(1);
    }

    log_producer_config * config = create_log_producer_config();
    // endpoint list:  https://help.aliyun.com/document_detail/29008.html
    log_producer_config_set_endpoint(config, "your-endpoint");
    log_producer_config_set_project(config, "your-project");
    log_producer_config_set_logstore(config, "your-logstore");

    // set credentials callback and userdata
    log_producer_config_set_credentials_callback(config, get_credentials_callback);
    log_producer_config_set_credentials_userdata(config, NULL);

    log_producer * producer =create_log_producer(config, on_log_send_done);
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

    for (int i = 0; i < 10; ++i)
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

    sleep(6);

    for (int i = 0; i < 10; ++i)
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

