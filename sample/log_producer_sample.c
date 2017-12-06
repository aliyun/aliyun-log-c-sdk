

//
// Created by ZhangCheng on 24/11/2017.
//

#include "log_producer_config.h"
#include "log_producer_client.h"



// 数据发送的回调函数
void on_log_send_done(const char * config_name, log_producer_result result, size_t log_bytes, size_t compressed_bytes, const char * req_id, const char * message)
{
    if (result == LOG_PRODUCER_OK)
    {
        printf("send done, config : %s, result : %s, log bytes : %d, compressed bytes : %d, request id : %s\n",
               config_name, get_log_producer_result_string(result),
               (int)log_bytes, (int)compressed_bytes, req_id);
        return;
    }
    printf("send error, config : %s, result : %s, log bytes : %d, compressed bytes : %d, request id : %s, error message : %s\n",
           config_name, get_log_producer_result_string(result),
           (int)log_bytes, (int)compressed_bytes, req_id, message);
    return;
}



void log_producer_post_logs(const char * fileName, int logs)
{
    // 初始化producer相关依赖环境
    if (log_producer_env_init() != LOG_PRODUCER_OK) {
        exit(1);
    }

    // 从指定配置文件创建producer实例，on_log_send_done 可以填NULL
    // 用户也可以手动创建config，使用create_log_producer接口创建producer
    log_producer * producer = create_log_producer_by_config_file(fileName, on_log_send_done);
    if (producer == NULL)
    {
        printf("create log producer by config file fail \n");
        exit(1);
    }

    // 获取默认的client
    log_producer_client * client = get_log_producer_client(producer, NULL);
    if (client == NULL)
    {
        printf("get root client fail \n");
        exit(1);
    }

    // 获取`error`的client
    log_producer_client * client_error = get_log_producer_client(producer, "error");
    if (client_error == NULL)
    {
        printf("get error client fail \n");
        exit(1);
    }


    int32_t i = 0;
    for (i = 0; i < logs; ++i)
    {
        // 使用C PRODUCER提供的默认宏接口直接发送日志。用户也可根据自己需求定制接口，具体可参见`log_producer_client`
        LOG_PRODUCER_INFO(client, "LogHub", "Real-time log collection and consumption",
                          "Search/Analytics", "Query and real-time analysis",
                          "Visualized", "dashboard and report functions",
                          "Interconnection", "Grafana and JDBC/SQL92");
        LOG_PRODUCER_ERROR(client_error, "error_type", "internal_server_error", "error_code", "501", "error_message", "unknow error message");
    }

    destroy_log_producer(producer);

    log_producer_env_destroy();
}

int main(int argc, char *argv[])
{
    const char * filePath = "./log_config.json";
    int logs = 100;
    if (argc == 3)
    {
        filePath = argv[1];
        logs = atoi(argv[2]);
    }
    log_producer_post_logs(filePath, logs);
    return 0;
}


