# Aliyun LOG C Producer 
日志服务C Producer Library是用纯C编写的日志采集客户端，为各类应用程序提供便捷、高性能、低资源占用的一站式数据采集方案，降低数据采集的开发与运维代价。

* 日志服务还提供Producer Lite版本，不依赖APR相关库且资源占用大大降低，具体请参见[Lite分支](https://github.com/aliyun/aliyun-log-c-sdk/tree/lite)

## 功能特点
* 异步
    * 异步写入，客户端线程无阻塞
* 聚合&压缩 上传
    * 支持按超时时间、日志数、日志size聚合数据发送
    * 支持lz4压缩
* 多客户端
	* 可同时配置多个客户端，每个客户端可独立配置采集优先级、缓存上限、目的project/logstore、聚合参数等
* 支持上下文查询
    * 同一个客户端产生的日志在同一上下文中，支持查看某条日志前后相关日志
* 并发发送
    * 支持可配置的线程池发送
* 缓存
    * 支持缓存上线可设置
    * 超过上限后日志写入失败
* 本地调试
    * 支持将日志内容输出到本地，并支持轮转、日志数、轮转大小设置
* 自定义标识
    * 支持设置自定义tag、topic

## 功能优势
* 客户端高并发写入：可配置的发送线程池，支持每秒数十万条日志写入，详情参见[性能测试](performance.md)。
* 低资源消耗：每秒20W日志写入只消耗70% CPU；同时在低性能硬件（例如树莓派）上，每秒产生100条日志对资源基本无影响。详情参见[性能测试](performance.md)。
* 客户端日志不落盘：既数据产生后直接通过网络发往服务端。
* 客户端计算与 I/O 逻辑分离：日志异步输出，不阻塞工作线程。
* 支持多优先级：不通客户端可配置不同的优先级，保证高优先级日志最先发送。
* 本地调试：支持设置本地调试，便于您在网络不通的情况下本地测试应用程序。

在以上场景中，C Producer Library 会简化您程序开发的步骤，您无需关心日志采集细节实现、也不用担心日志采集会影响您的业务正常运行，大大降低数据采集门槛。

![image.png](http://ata2-img.cn-hangzhou.img-pub.aliyun-inc.com/bb6a86424ebba0efeca634dc2cc2ffd3.png)

## 安装方法
### 环境依赖
LOG C SDK使用curl进行网络操作，无论是作为客户端还是服务器端，都需要依赖curl。
另外，LOG C SDK使用apr/apr-util库解决内存管理以及跨平台问题。
LOG C SDK并没有带上这几个外部库，您需要确认这些库已经安装，并且将它们的头文件目录和库文件目录都加入到了项目中。

* 日志服务还提供Producer Lite版本，不依赖APR相关库且资源占用大大降低，具体请参见[Lite分支](https://github.com/aliyun/aliyun-log-c-sdk/tree/lite)

#### 第三方库下载以及安装

##### libcurl （建议 7.32.0 及以上版本）

  请从[这里](http://curl.haxx.se/download.html)下载，并参考[libcurl 安装指南](http://curl.haxx.se/docs/install.html)安装。典型的安装方式如下：
```shell
    ./configure
    make
    make install
```

注意：
 - 执行./configure时默认是配置安装目录为/usr/local/，如果需要指定安装目录，请使用 ./configure --prefix=/your/install/path/

##### apr （建议 1.5.2 及以上版本）

  请从[这里](https://apr.apache.org/download.cgi)下载，典型的安装方式如下：
 ```shell
    ./configure
    make
    make install
```

注意：
 - 执行./configure时默认是配置安装目录为/usr/local/，如果需要指定安装目录，请使用 ./configure --prefix=/your/install/path/

##### apr-util （建议 1.5.4 及以上版本）

  请从[这里](https://apr.apache.org/download.cgi)下载，安装时需要注意指定--with-apr选项，典型的安装方式如下：
```shell
    ./configure --with-apr=/your/apr/install/path
    make
    make install
```

注意：
 - 执行./configure时默认是配置安装目录为/usr/local/，如果需要指定安装目录，请使用 ./configure --prefix=/your/install/path/
 - 需要通过--with-apr指定apr安装目录，如果apr安装到系统目录下需要指定--with-apr=/usr/local/apr/


##### CMake (建议2.6.0及以上版本)

  请从[这里](https://cmake.org/download)下载，典型的安装方式如下：
```shell
    ./configure
    make
    make install
```

注意：
 - 执行./configure时默认是配置安装目录为/usr/local/，如果需要指定安装目录，请使用 ./configure --prefix=/your/install/path/

#### LOG C SDK的安装

  安装时请在cmake命令中指定第三方库头文件以及库文件的路径，典型的编译命令如下：
```shell
    cmake .
    make
    make install
```

注意：
 - 执行cmake . 时默认会到/usr/local/下面去寻找curl，apr，apr-util的头文件和库文件。
 - 默认编译是Debug类型，可以指定以下几种编译类型： Debug, Release, RelWithDebInfo和MinSizeRel，如果要使用release类型编译，则执行cmake . -DCMAKE_BUILD_TYPE=Release
 - 如果您在安装curl，apr，apr-util时指定了安装目录，则需要在执行cmake时指定这些库的路径，比如：
```shell
   cmake . -DCURL_INCLUDE_DIR=/usr/local/include/curl/ -DCURL_LIBRARY=/usr/local/lib/libcurl.a -DAPR_INCLUDE_DIR=/usr/local/include/apr-1/ -DAPR_LIBRARY=/usr/local/lib/libapr-1.a -DAPR_UTIL_INCLUDE_DIR=/usr/local/apr/include/apr-1 -DAPR_UTIL_LIBRARY=/usr/local/apr/lib/libaprutil-1.a
```
 - 如果要指定安装目录，则需要在cmake时增加： -DCMAKE_INSTALL_PREFIX=/your/install/path/usr/local/

## 使用
一个应用可创建多个producer，每个producer共享一个发送线程池；一个producer可包含多个client，每个client可单独配置目的地址、日志level、是否本地调试、缓存大小、自定义标识、topic等信息。

![image.png](http://ata2-img.cn-hangzhou.img-pub.aliyun-inc.com/955c4d62349b893f2bff22a9f5308a48.png)
### 自定义配置文件
配置文件为JSON格式，样例配置如下，详细参数含义以及默认值见本文档中参数说明部分。
```json
{
    "send_thread_count" : 4,
    "endpoint" : "http://cn-shanghai-intranet.log.aliyun-inc.com",
    "project" : "xxxxxxx",
    "logstore" : "xxxxxxx",
    "access_id" : "xxxxxxxxxx",
    "access_key" : "xxxxxxxxxxxxxxxxxx",
    "name" : "root",
    "topic" : "topic_test",
    "tags" : {
        "tag_key" : "tag_value"
    },
    "level" : "INFO",
    "priority" : "NORMAL",
    "package_timeout_ms" : 3000,
    "log_count_per_package" : 4096,
    "log_bytes_per_package" : 3000000,
    "max_buffer_bytes" : 100000000,
    "debug_open" : 1,
    "debug_stdout" : 0,
    "debug_log_path" : "./log_debug.log",
    "max_debug_logfile_count" : 5,
    "max_debug_logfile_size" : 1000000,
    "sub_appenders" : {
        "error" : {
                        "endpoint" : "http://cn-shanghai.log.aliyun.com",
                        "project" : "xxxxxxx",
                        "logstore" : "xxxxxxx",
                        "access_id" : "xxxxxxxxxx",
                        "access_key" : "xxxxxxxxxxxxxxxxxx",
                        "name" : "error",
                        "topic" : "topic_xxx",
                        "level" : "INFO",
                        "priority" : "HIGH",
                        "package_timeout_ms" : 3000,
                        "log_count_per_package" : 4096,
                        "max_buffer_bytes" : 100000000
				  }
		}
}
```
### 使用接口发送日志
代码示例如下：
```c
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

    // 获取`error`的client，如果不存在`error`的client，则返回默认client
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

```

### 日志样例
上一节的示例代码发送到日志服务后的日志样例如下，除了将用户字段上传外，日志服务还会自动上传调用日志接口的文件名、行数、线程id、函数名、当前时间、配置的自定义tag、topic等。
```
__source__:  11.164.233.187  
__tag__:tag_key:  tag_value  
__topic__:  topic_test  
_file_:  /disk1/workspace/tools/aliyun-log-c-sdk/sample/log_producer_benchmark.c  
_function_:  log_producer_post_logs  
_level_:  LOG_PRODUCER_LEVEL_ERROR  
_line_:  69  
_thread_:  59287424  
error_code:  501  
error_message:  unknow error message  
error_type:  internal_server_error  


__source__:  11.164.233.187   
__topic__:  topic_xxx  
_file_:  /disk1/workspace/tools/aliyun-log-c-sdk/sample/log_producer_benchmark.c  
_function_:  log_producer_post_logs  
_level_:  LOG_PRODUCER_LEVEL_INFO  
_line_:  68  
_thread_:  59287424  
Interconnection:  Grafana and JDBC/SQL92  
LogHub:  Real-time log collection and consumption  
Search/Analytics:  Query and real-time analysis  
Visualized:  dashboard and report functions  
```

### 详细配置
C Producer Library配置模式如下，除`send_thread_count`为producer独有配置，其他属性每个客户端都可以独立配置。

* JSON根目录下配置`send_thread_count`和默认的client，其他client在`sub_appenders`字段中配置
* **注意：** `sub_appenders`下的key需要与配置中的`name`字段一致

![image.png](http://ata2-img.cn-hangzhou.img-pub.aliyun-inc.com/90d0f0259e38144f560a5e588572ce14.png)

| 参数                     | 说明                                                                                 | 取值                                    |
| ------------------------ | ------------------------------------------------------------------------------------ | --------------------------------------- |
| send_thread_count    | 指定发送线程池最大线程数量，用于发送数据到日志服务。 **注意:producer下所有client共享线程池，线程池基于优先级调度**                             | 整数形式，默认1。                              |
| project | 日志服务project | string |
| logstore | 日志服务logstore | string |
| endpoint | 日志服务endpoint | string，以`http:`开头 |
| access_id | 云账号 access id | string |
| access_key | 云账号 access key | string |
| sts_token | sts tokey，**注意: access id&key 和 sts token 只需填写一个，sts token具有一定的有效期，如果设置sts token，需要您在程序中定期切换有效token，具体方案可参考[sts token服务器搭建](https://help.aliyun.com/document_detail/62681.html)** | string |
| name | 配置名，用户获取client | string |
| topic | topic名 | string |
| tags | 自定义的tag | JSON object 例如 `{"tag_key" : "tag_value", "tag_key1" : "tag_value1"}`
| priority | 配置优先级 | 取值范围：LOWEST, LOW, NORMAL, HIGH, HIGHEST |
| level | 最低输出的level，低于此level的数据将不会处理，直接丢弃 | 取值范围：DEBUG、INFO、WARN、ERROR、FATAL |
| package_timeout_ms       | 指定被缓存日志的发送超时时间，如果缓存超时，则会被立即发送。                         | 整数形式，单位为毫秒。                  |
| log_count_per_package      | 指定每个缓存的日志包中包含日志数量的最大值。                                         | 整数形式，取值为1~4096。                |
| log_bytes_per_package      | 指定每个缓存的日志包的大小上限。                                                     | 整数形式，取值为1~5242880，单位为字节。 |
| max_buffer_bytes        | 指定单个Producer Client实例可以使用的内存的上限。                                           | 整数形式，单位为字节。                  |
| debug_open | 是否开启debug模式，开启debug模式时会将日志写入到本地                                                           | 0关闭，1开启。默认0                    |
| debug_stdout | debug模式下是否将日志打印到stdout                                                           | 0否，1是。默认0                    |
| debug_log_path             | 调试时日志本地保存文件名                                                             | 字符串，为debug保存日志全路径           |
| max_debug_logfile_count     | 调试时最大debug日志个数                                                              | 整数                                    |
| max_debug_logfile_size      | 调试时单一日志文件最大大小                                                           | 整数，单位字节                          |

## 底层日志发送接口
若producer提供的接口满足不了您的日志采集需求，您可以基于底层的[日志发送接口](inner_interface.md)，开发适合您的应用场景的日志采集API。

## 联系我们
- [阿里云LOG官方网站](https://www.aliyun.com/product/sls/)
- [阿里云LOG官方论坛](https://yq.aliyun.com/groups/50)
- 阿里云官方技术支持：[提交工单](https://workorder.console.aliyun.com/#/ticket/createIndex)
