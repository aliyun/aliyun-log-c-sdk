# 服务端测试
## 环境
### 软硬件环境
* CPU : Intel(R) Xeon(R) CPU E5-2682 v4 @ 2.50GHz
* MEM : 64GB
* OS : Linux version 2.6.32-220.23.2.ali1113.el5.x86_64
* GCC : 4.1.2
* c-producer：动态库 65K、静态库59K (测试使用静态库，编译后的binary 69KB，所有都是strip后)

### c-producer配置
* 缓存：64MB
* 聚合时间：3秒  （聚合时间、聚合数据包大小、聚合日志数任一满足即打包发送）
* 聚合数据包大小：4MB
* 聚合日志数：4096
* 发送线程：16
* 自定义tag : 5

### 日志样例
日志样例为2种
1. 10个键值对，总数据量约为700字节
1. 4个键值对，数据量约为200字节

## 测试结果

| 发送速率(logs/s) | 平均每次log耗时(us) | 原始数据速率(KB/s) | cpu(%) | mem(MB) |
| ---------------- | ---------------- | ------------------- | ------------------ | ------ |
| 200000           | 1.08                | 84495.5           | 28.8   | 10.58  |
| 100000           | 1.10                | 42247.8            | 14.6   | 8.10    |
| 20000            | 1.09                | 8449.6             | 3.0    | 7.35    |
| 10000            | 1.08                 | 4224.7             | 1.6    | 5.12    |
| 2000             | 1.20                | 844.9              | 0.2    | 3.58    |
| 1000             | 1.19                | 463.9              | 0.1    | 3.31     |
| 200              | 1.45（不准确）     | 84.5               | 0.0?   | 2.81    |
| 100              | 1.70（不准确）     | 42.2               | 0.0?   | 2.82    |

# 树莓派测试
## 软硬件环境
### 硬件环境
* 型号：树莓派3B
* CPU：Broadcom BCM2837 1.2GHz A53 64位(使用主机USB供电，被降频到600MHz)
* 内存：1GB DDR2
* 网络：百兆网卡（台式机本地网卡提供共享网络，台式机网络为无线网，网络质量不是很好，经常还有timeout）

### OS&软件
* OS: Linux 4.9.41-v7+ #1023 SMP armv71 GNU/Linux
* GCC：6.3.0 (Raspbian 6.3.0-18+rpi1)
* c-producer：动态库 72K、静态库65K (测试使用静态库，编译后的binary 87KB，所有都是strip后)

### c-producer配置
* 缓存：10MB
* 聚合时间：3秒  （聚合时间、聚合数据包大小、聚合日志数任一满足即打包发送）
* 聚合数据包大小：1MB
* 聚合日志数：1000
* 发送线程：4
* 自定义tag : 5

### 日志样例
日志样例为2种
1. 10个键值对，总数据量约为700字节
1. 4个键值对，数据量约为200字节

## 测试结果

| 发送速率(logs/s) | 平均每次log耗时(us) | 原始数据速率(KB/s) | cpu(%) | mem(MB) |
| ---------------- | ---------------- | ------------------- | ------------------ | ------ |
| 20000            | 8.8                | 8449.6             | 31.5    | 5.46    |
| 10000            | 9.3                 | 4224.7             | 15.8    | 5.1    |
| 2000             | 9.9                | 844.9              | 2.9    | 3.38    |
| 1000             | 10.5                | 463.9              | 1.4    | 3.11     |
| 200              | 10.1                 | 84.5               | 0.3   | 2.41    |
| 100              | 10.6                | 42.2               | 0.1   | 3.01    |
| 20               | 10.3                | 8.4               | 0.0?   | 2.96    |
| 10               | 11.1               | 4.2               | 0.0?   | 2.80    |

# 总结
* 服务器端的测试中，C Producer可以轻松到达90M/s的发送速度，每秒上传日志20W，占用CPU只有70%，内存12M
* 在树莓派的测试中，由于CPU的频率只有600MHz，性能差不多是服务器的1/10左右，最高每秒可发送2W条日志
* 服务器在200条/s、树莓派在20条/s的时候，发送数据对于cpu基本无影响（降低到0.01%以内）
* 客户线程发送一条数据（输出一条log）的平均耗时为：服务器1.1us、树莓派10us左右


# 附件

## 日志样例
```
Interconnection:Grafana and JDBC/SQL92
LogHub:Real-time log collection and consumption
Search/Analytics:Query and real-time analysis
Visualized:dashboard and report functions
__tag__:__pack_id__:F20B4E7940C80975-0
__tag__:tag_1:val_1
__tag__:tag_2:val_2
__tag__:tag_3:val_3
__tag__:tag_4:val_4
__tag__:tag_5:val_5
__topic__:test_topic
```

```
__tag__:__pack_id__:F20B4E7940C80975-0
__tag__:tag_1:val_1
__tag__:tag_2:val_2
__tag__:tag_3:val_3
__tag__:tag_4:val_4
__tag__:tag_5:val_5
__topic__:test_topic
content_key_1:1abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+
content_key_2:2abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+
content_key_3:3abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+
content_key_4:4abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+
content_key_5:5abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+
content_key_6:6abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+
content_key_7:7abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+
content_key_8:8abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+
content_key_9:9abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+
index:0
```

## 测试代码
```
#include "inner_log.h"
#include "log_api.h"
#include "log_producer_config.h"
#include "log_producer_client.h"
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include "log_multi_thread.h"


void on_log_send_done(const char * config_name, log_producer_result result, size_t log_bytes, size_t compressed_bytes, const char * req_id, const char * message)
{
    if (result == LOG_PRODUCER_OK)
    {
        if (aos_log_level == AOS_LOG_DEBUG)
        {
            printf("send success, config : %s, result : %d, log bytes : %d, compressed bytes : %d, request id : %s \n",
                   config_name, (result),
                   (int)log_bytes, (int)compressed_bytes, req_id);
        }

    }
    else
    {
        printf("send fail, config : %s, result : %d, log bytes : %d, compressed bytes : %d, request id : %s, error message : %s\n",
               config_name, (result),
               (int)log_bytes, (int)compressed_bytes, req_id, message);
    }

}

log_producer * create_log_producer_wrapper(on_log_producer_send_done_function on_send_done)
{
    log_producer_config * config = create_log_producer_config();
    // endpoint list:  https://help.aliyun.com/document_detail/29008.html
    log_producer_config_set_endpoint(config, "${your_endpoint}");
    log_producer_config_set_project(config, "${your_project}");
    log_producer_config_set_logstore(config, "${your_logstore}");
    log_producer_config_set_access_id(config, "${your_access_key_id}");
    log_producer_config_set_access_key(config, "${your_access_key_secret}");

    // if you do not need topic or tag, comment it
    log_producer_config_set_topic(config, "test_topic");
    log_producer_config_add_tag(config, "tag_1", "val_1");
    log_producer_config_add_tag(config, "tag_2", "val_2");
    log_producer_config_add_tag(config, "tag_3", "val_3");
    log_producer_config_add_tag(config, "tag_4", "val_4");
    log_producer_config_add_tag(config, "tag_5", "val_5");

    // set resource params
    log_producer_config_set_packet_log_bytes(config, 4*1024*1024);
    log_producer_config_set_packet_log_count(config, 4096);
    log_producer_config_set_packet_timeout(config, 3000);
    log_producer_config_set_max_buffer_limit(config, 64*1024*1024);

    // set send thread
    log_producer_config_set_send_thread_count(config, 16);

    return create_log_producer(config, on_send_done);
}

void log_producer_post_logs(int logsPerSecond, int sendSec)
{
    //aos_log_level = AOS_LOG_DEBUG;
    if (log_producer_env_init(LOG_GLOBAL_ALL) != LOG_PRODUCER_OK) {
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
        int64_t startTime = GET_TIME_US();
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

            log_producer_result rst = log_producer_client_add_log(client, 8, "LogHub", "Real-time log collection and consumption",
                              "Search/Analytics", "Query and real-time analysis",
                              "Visualized", "dashboard and report functions",
                              "Interconnection", "Grafana and JDBC/SQL92");
            if (rst != LOG_PRODUCER_OK)
            {
                printf("add log error %d \n", rst);
            }
        }
        int64_t endTime = GET_TIME_US();
        aos_error_log("Done : %d  %d time  %f us \n", i, logsPerSecond, (float)(endTime - startTime) * 1.0 / logsPerSecond / 2);
        totalTime += endTime - startTime;
        if (endTime - startTime < 1000000)
        {
            usleep(1000000 - (endTime - startTime));
        }
    }
    aos_error_log("Total done : %f us, avg %f us", (float)totalTime / 180, (float)totalTime / (180 * logsPerSecond * 2));
    //sleep(10);

    destroy_log_producer(producer);

    log_producer_env_destroy();
}

int main(int argc, char *argv[])
{
    int logsPerSec = 100;
    int sendSec = 180;
    if (argc == 3)
    {
        logsPerSec = atoi(argv[1]);
        sendSec = atoi(argv[2]);
    }
    log_producer_post_logs(logsPerSec, sendSec);
    return 0;
}


``` 