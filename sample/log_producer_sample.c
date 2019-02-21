

//
// Created by ZhangCheng on 24/11/2017.
//

#include "log_producer_config.h"
#include "log_producer_client.h"

int useStsToken = 0;
const char * accessId = "STS.addfadsafdsfdsfadsfsdfds";
const char * accessKey = "G8VmGs9zhV2CbdC123213213123123123fdsaf";
const char * token = "CAIStQJ1q6Ft5B2yfSjIr4vXecnbhrAT7qihM+=";

// 数据发送的回调函数
// @note producer后台会对多条日志进行聚合发送，每次发送无论成功或者失败都会通过此函数回调，所以回调中可能是一组日志
// @note 发送失败时，如果是LOG_PRODUCER_SEND_SERVER_ERROR、LOG_PRODUCER_SEND_TIME_ERROR、LOG_PRODUCER_SEND_QUOTA_ERROR、LOG_PRODUCER_SEND_NETWORK_ERROR，producer后台会进行重试。若超过6小时数据包未发送成功，则丢弃该数据
// @note 如果您需要查看失败的日志内容，可以使用deserialize_from_lz4_proto_buf(buffer, compressed_bytes, log_bytes) 来解析日志，具体请参见示例代码
// @note 如果返回值是LOG_PRODUCER_SEND_EXIT_BUFFERED，说明程序正在退出中，此buffer将不会被发送，建议您将其存储到本地，下次程序启动时再调用 log_producer_client_add_raw_log_buffer 将其发送出去
// @note 此函数可能被多线程调用，注意线程安全
// @note 注意判断 req_id  message 是否为空，printf打印NULL的行为可能是不确定的

void on_log_send_done(const char * config_name, log_producer_result result, size_t log_bytes, size_t compressed_bytes, const char * req_id, const char * message, const unsigned char * buffer, size_t log_num)
{
    // @note 正常使用时，建议result为LOG_PRODUCER_OK时不要打印
    if (result == LOG_PRODUCER_OK)
    {
        if (req_id == NULL)
        {
            req_id = "";
        }
        printf("send done, config : %s, result : %s, log bytes : %d, compressed bytes : %d, request id : %s, log number : %d\n",
               config_name, get_log_producer_result_string(result),
               (int)log_bytes, (int)compressed_bytes, req_id, (int)log_num);
        return;
    }

    if (req_id == NULL)
    {
        req_id = "";
    }
    if (message == NULL)
    {
        message = "";
    }

    printf("send error, config : %s, result : %s, log bytes : %d, compressed bytes : %d, request id : %s, error message : %s, log number : %d\n",
           config_name, get_log_producer_result_string(result),
           (int)log_bytes, (int)compressed_bytes, req_id, message, (int)log_num);


    // 以下代码实现的功能是：当发送失败时，将失败日志的前10条打印出来
    log_group * logGroup = deserialize_from_lz4_proto_buf(buffer, compressed_bytes, log_bytes);
    if (logGroup == NULL)
    {
        return;
    }
    size_t i = 0;
    printf("######    error logs info        ######\n");
    printf("######    tags count : %06d    ######\n", (int)logGroup->n_logtags);
    for (i = 0; i < logGroup->n_logtags; ++i)
    {
        printf("\t %s : %s\n", logGroup->logtags[i]->key, logGroup->logtags[i]->value);
    }
    printf("######    logs count : %06d    #####\n", (int)logGroup->n_logs);
    size_t maxPrintLogs = logGroup->n_logs > (size_t)10 ? 10 : logGroup->n_logs;
    for (i = 0; i < maxPrintLogs; ++i)
    {
        printf("log -> %06d \n", (int)i);
        log_lg * log = logGroup->logs[i];
        size_t j = 0;
        for (j = 0; j < log->n_contents; ++j)
        {
            printf("\t %s : %s \n", log->contents[j]->key, log->contents[j]->value);
        }
    }
    printf("######    done                   ######\n");
    log_group_free(logGroup, NULL);
    // done
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

#ifdef __linux__
    printf("client thread id %d \n", (int)log_producer_client_get_flush_thread(client));
#endif

    // 获取`error`的client
    log_producer_client * client_error = get_log_producer_client(producer, "error");
    if (client_error == NULL)
    {
        printf("get error client fail \n");
        exit(1);
    }

#ifdef __linux__
    printf("client_error id %d \n", (int)log_producer_client_get_flush_thread(client));
#endif

    // 如果需要动态设置 sts token，调用如下接口
    if (useStsToken)
    {
        log_producer_client_update_token(client, accessId, accessKey, token);
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
    // for debug
    //aos_log_set_level(AOS_LOG_ALL);
    const char * filePath = "./log_config.json";
    int logs = 100;
    if (argc == 3)
    {
        filePath = argv[1];
        logs = atoi(argv[2]);
    }
    int i =0;
    for(i = 0; i < 3; ++i)
    {
        log_producer_post_logs(filePath, logs);
    }
    return 0;
}


