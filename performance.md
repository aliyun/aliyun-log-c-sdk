

# 服务端测试
## 环境
### 软硬件环境
* CPU : Intel(R) Xeon(R) CPU E5-2682 v4 @ 2.50GHz
* MEM : 64GB
* OS : Linux version 2.6.32-220.23.2.ali1113.el5.x86_64
* GCC : 4.1.2
* c-producer：动态库 162K、静态库140K (测试使用静态库，编译后的binary 157KB，所有都是strip后)

### c-producer配置
* 缓存：10MB
* 聚合时间：3秒  （聚合时间、聚合数据包大小、聚合日志数任一满足即打包发送）
* 聚合数据包大小：3MB
* 聚合日志数：4096
* 发送线程：4
* 自定义tag : 5

### 日志样例
日志样例为2种
1. 10个键值对，总数据量约为600字节
1. 9个键值对，数据量约为350字节

## 测试结果

| 发送速率(logs/s) | 用户线程耗时(us) | 平均每次log耗时(us) | 原始数据速率(KB/s) | cpu(%) | mem(MB) |
| ---------------- | ---------------- | ------------------- | ------------------ | ------ | ------- |
| 200000           | 225000           | 1.12                | 92773.4            | 67.9   | 140.98  |
| 100000           | 118000           | 1.18                | 46386.7            | 30.9   | 69.1    |
| 20000            | 23800            | 1.19                | 9277.3             | 5.3    | 24.2    |
| 10000            | 12000            | 1.2                 | 4638.6             | 2.7    | 18.2    |
| 2000             | 2560             | 1.28                | 927.7              | 0.7    | 10.3    |
| 1000             | 1246             | 1.24                | 463.9              | 0.3    | 8.1     |
| 200              | 284              | 1.42?（不准确）     | 92.7               | 0.0?   | 4.45    |
| 100              | 159              | 1.59?（不准确）     | 46.4               | 0.0?   | 3.98    |

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
* c-producer：动态库 179K、静态库162K (测试使用静态库，编译后的binary 287KB，所有都是strip后)

### c-producer配置
* 缓存：10MB
* 聚合时间：3秒  （聚合时间、聚合数据包大小、聚合日志数任一满足即打包发送）
* 聚合数据包大小：1MB
* 聚合日志数：1000
* 发送线程：1
* 自定义tag : 5

### 日志样例
日志样例为2种
1. 10个键值对，总数据量约为600字节
1. 9个键值对，数据量约为350字节

## 测试结果

| 发送速率(logs/s) | 用户线程耗时(us) | 平均每次log耗时(us) | 原始数据速率(KB/s) | cpu(%) | mem(MB) |
| ---------------- | ---------------- | ------------------- | ------------------ | ------ | ------- |
| 20000            | 175000           | 8.75                | 9277.3             | 44.9   | 13.98   |
| 10000            | 95000            | 9.5                 | 4638.7             | 26.5   | 9.45    |
| 2000             | 32000            | 16                  | 927.73             | 6.5    | 6.3     |
| 1000             | 12000            | 12                  | 463.86             | 3.3    | 5.32    |
| 200              | 2270             | 11.3                | 92.77              | 0.7    | 4.22    |
| 100              | 1130             | 11.3                | 46.39              | 0.3    | 3.86    |
| 20               | 235              | 11.7                | 9.27               | 0.0?   | 3.4     |
| 10               | 132              | 13.2                | 4.64               | 0.0?   | 3.2     |

# 总结
* 服务器端的测试中，C Producer可以轻松到达90M/s的发送速度，每秒上传日志20W，占用CPU只有70%，内存140M
* 在树莓派的测试中，由于CPU的频率只有600MHz，性能差不多是服务器的1/10左右，最高每秒可发送2W条日志
* 服务器在200条/s、树莓派在20条/s的时候，发送数据对于cpu基本无影响（降低到0.01%以内）
* 客户线程发送一条数据（输出一条log）的平均耗时为：服务器1.2us、树莓派12us左右
* 设置的缓存大小和程序实际占用大小最多会有1.5倍左右差距（多出的占用主要因为内存碎片化以及保存pb结构信息）
* 由于发送使用线程池，目前来看性能瓶颈在于单线程的序列化&压缩，可优化点：线程池处理；手写pb序列化

# 附件
## 配置样例
```
{
    "endpoint" : "http://cn-shanghai-intranet.log.aliyun-inc.com",
    "project" : "xxxxxxx",
    "logstore" : "xxxxxxx",
    "access_id" : "xxxxxxxxxx",
    "access_key" : "xxxxxxxxxxxxxxxxxx",
    "name" : "test_config",
    "topic" : "topic_test",
    "tags" : {
        "a" : "b",
        "c" : "d",
        "tag_key" : "tag_value",
        "1": "2",
        "3" : 4,
        "5" : "6"
    },
    "level" : "INFO",
    "package_timeout_ms" : 3000,
    "log_count_per_package" : 4096,
    "log_bytes_per_package" : 3000000,
    "retry_times" : 10,
    "send_thread_count" : 4,
    "max_buffer_bytes" : 100000000,
    "debug_open" : 0,
    "debug_stdout" : 0,
    "debug_log_path" : "./log_debug.log",
    "max_debug_logfile_count" : 5,
    "max_debug_logfile_size" : 1000000
}
```

## 日志样例
```
__source__:  11.164.233.187  
__tag__:1:  2  
__tag__:5:  6  
__tag__:a:  b  
__tag__:c:  d  
__tag__:tag_key:  tag_value  
__topic__:  topic_test  
_file_:  /disk1/workspace/tools/aliyun-log-c-sdk/sample/log_producer_sample.c  
_function_:  log_producer_post_logs  
_level_:  LOG_PRODUCER_LEVEL_WARN  
_line_:  248  
_thread_:  40978304  
LogHub:  Real-time log collection and consumption  
Search/Analytics:  Query and real-time analysis  
Interconnection:  Grafana and JDBC/SQL92  
Visualized:  dashboard and report functions  
```
## 

```
__source__:  11.164.233.187  
__tag__:1:  2  
__tag__:5:  6  
__tag__:a:  b  
__tag__:c:  d  
__tag__:tag_key:  tag_value  
__topic__:  topic_test  
content_key_1:  1abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+  
content_key_2:  2abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+  
content_key_3:  3abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+  
content_key_4:  4abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+  
content_key_5:  5abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+  
content_key_6:  6abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+  
content_key_7:  7abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+  
content_key_8:  8abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+  
content_key_9:  9abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+  
index:  xxxxxxxxxxxxxxxxxxxxx  
```

## 测试代码
```
#include "aos_log.h"
#include "aos_util.h"
#include "aos_string.h"
#include "aos_status.h"
#include "log_auth.h"
#include "log_util.h"
#include "log_api.h"
#include "log_config.h"
#include "log_producer_config.h"
#include "log_producer_debug_flusher.h"
#include "log_producer_client.h"
#include <time.h>
#include <stdlib.h>

void log_producer_post_logs(const char * fileName, int logsPerSecond)
{
    //aos_log_level = AOS_LOG_DEBUG;
    if (log_producer_env_init() != LOG_PRODUCER_OK) {
        exit(1);
    }

    log_producer * producer = create_log_producer_by_config_file(fileName, NULL);
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

    log_producer_client * client3 = get_log_producer_client(producer, "order.error");
    if (client3 == NULL)
    {
        printf("create log producer client by config file fail \n");
        exit(1);
    }

    //assert(client != client2);
    //assert(client != client3);

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

        	LOG_PRODUCER_WARN(client2, "LogHub", "Real-time log collection and consumption",
                              "Search/Analytics", "Query and real-time analysis",
                              "Visualized", "dashboard and report functions",
                              "Interconnection", "Grafana and JDBC/SQL92");
        	LOG_PRODUCER_ERROR(client3, "a", "v", "c", "a", "v");
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

    destroy_log_producer(producer);

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
    log_producer_post_logs(filePath, logsPerSec);
    return 0;
}
``` 