 日志服务C Producer Lite是用纯C编写的日志采集客户端，提供更加精简的环境依赖以及更低的资源占用，试用于各类嵌入式/智能设备的日志采集。
 
# 功能特点

* 异步
    * 异步写入，客户端线程无阻塞
* 聚合&压缩 上传
    * 支持按超时时间、日志数、日志size聚合数据发送
    * 支持lz4压缩
* 支持上下文查询
    * 同一个客户端产生的日志在同一上下文中，支持查看某条日志前后相关日志
* 并发发送
    * 支持可配置的线程池发送
* 缓存
    * 支持缓存上线可设置
    * 超过上限后日志写入失败
* 自定义标识
    * 日志上传时默认会带上ip
    * 支持设置自定义tag、topic

# 功能优势

* 客户端高并发写入：可配置的发送线程池，支持每秒数十万条日志写入，详情参见[性能测试](performance.md)。
* 低资源消耗：每秒20W日志写入只消耗30% CPU；同时在低性能硬件（例如树莓派）上，每秒产生100条日志对资源基本无影响。详情参见[性能测试](performance.md)。
* 客户端日志不落盘：数据产生后直接通过网络发往服务端。
* 客户端计算与 I/O 逻辑分离：日志异步输出，不阻塞工作线程。

在以上场景中，C Producer Lite 会简化您程序开发的步骤，您无需关心日志采集细节实现、也不用担心日志采集会影响您的业务正常运行，大大降低数据采集门槛。

# 与C Producer对比
## 功能对比
相比C Producer，仅缺少以下功能：
1. 同一Producer只支持单个Client，不支持多Client设置
2. 不支持发送优先级设置
3. 不支持JSON方式的配置文件
4. 不提供日志相关宏定义包装
5. 不支持本地调试功能

## 性能对比
性能相对C Producer有30%以上提升。

## 资源占用对比
资源占用相比C Producer大大降低（详细请参考[性能测试](performance.md)）：
* 除去libcurl占用，运行期间内存占用80KB左右。
* 编译后的动态库仅有65KB（strip后）

## 安装方法
### 下载Producer Lite代码
Producer Lite代码在lite分支，您可以使用如下命令获取代码：
```shell
git clone https://github.com/aliyun/aliyun-log-c-sdk.git -b lite
```

### 环境依赖
C Producer Lite使用curl进行网络操作，您需要确认这些库已经安装，并且将它们的头文件目录和库文件目录都加入到了项目中。

#### libcurl下载以及安装

  libcurl建议 7.32.0 及以上版本

  请从[这里](http://curl.haxx.se/download.html)下载，并参考[libcurl 安装指南](http://curl.haxx.se/docs/install.html)安装。典型的安装方式如下：
```shell
    ./configure
    make
    make install
```

* 注意：
 - 执行./configure时默认是配置安装目录为/usr/local/，如果需要指定安装目录，请使用 ./configure --prefix=/your/install/path/

#### LOG C SDK的安装

  安装时请在cmake命令中指定第三方库头文件以及库文件的路径，典型的编译命令如下：
```shell
    cmake .
    make
    make install
```


* 注意：
 - 执行cmake . 时默认会到/usr/local/下面去寻找curl的头文件和库文件。
 - 默认编译是Debug类型，可以指定以下几种编译类型： Debug, Release, RelWithDebInfo和MinSizeRel，如果要使用release类型编译，则执行cmake . -DCMAKE_BUILD_TYPE=Release
 - 如果您在安装curl时指定了安装目录，则需要在执行cmake时指定这些库的路径，比如：
```shell
   cmake . -DCURL_INCLUDE_DIR=/usr/local/include/curl/ -DCURL_LIBRARY=/usr/local/lib/libcurl.a
```
 - 如果要指定安装目录，则需要在cmake时增加： -DCMAKE_INSTALL_PREFIX=/your/install/path/usr/local/

## 使用
一个应用可创建多个producer，每个producer可单独配置目的地址、缓存大小、发送线程数、自定义标识、topic等信息。

### 配置参数
所有参数通过`log_producer_config`结构设置，支持设置的参数如下：

| 参数                     | 说明                                                                                 | 取值                                    |
| ------------------------ | ------------------------------------------------------------------------------------ | --------------------------------------- |
| project | 日志服务project | string |
| logstore | 日志服务logstore | string |
| endpoint | 日志服务endpoint | string，以`http:`开头 |
| access_id | 云账号 access id | string |
| access_key | 云账号 access key | string |
| topic | topic名 | string，非必须设置。 |
| add_tag | 自定义的tag | key value 模式，非必须设置。 |
| packet_timeout       | 指定被缓存日志的发送超时时间，如果缓存超时，则会被立即发送。                         | 整数形式，单位为毫秒。默认为3000。                  |
| packet_log_count      | 指定每个缓存的日志包中包含日志数量的最大值。                                         | 整数形式，取值为1~4096。默认2048。               |
| packet_log_bytes      | 指定每个缓存的日志包的大小上限。                                                  | 整数形式，取值为1~5242880，单位为字节。默认为 3 * 1024 * 1024。 |
| max_buffer_limit        | 指定单个Producer Client实例可以使用的内存的上限。                                           | 整数形式，单位为字节。默认为64 * 1024 * 1024。                  |
| send_thread_count    | 指定发送线程数量，用于发送数据到日志服务。 | 整数形式，默认0，为0时发送与内部Flush公用一个线程，若您每秒日志发送不超过100条，无需设置此参数。                              |
| net_interface | 网络发送绑定的网卡名 | 字符串，为空时表示自动选择可用网卡 |
| connect_timeout_sec | 网络连接超时时间 | 整数，单位秒，默认为10|
| send_timeout_sec|日志发送超时时间|整数，单位秒，默认为15|
| destroy_flusher_wait_sec | flusher线程销毁最大等待时间 | 整数，单位秒，默认为1|
| destroy_sender_wait_sec | sender线程池销毁最大等待时间 | 整数，单位秒，默认为1|
| compress_type | 数据上传时的压缩类型，默认为LZ4压缩 | 0 不压缩，1 LZ4压缩， 默认为1|
| ntp_time_offset | 设备时间与标准时间之差，值为标准时间-设备时间，一般此种情况用户客户端设备时间不同步的场景 | 整数，单位秒，默认为0；比如当前设备时间为1607064208, 标准时间为1607064308，则值设置为 1607064308 - 1607064208 = 100  |
| max_log_delay_time | 日志时间与本机时间之差，超过该大小后会根据 `drop_delay_log` 选项进行处理。一般此种情况只会在设置persistent的情况下出现，即设备下线后，超过几天/数月启动，发送退出前未发出的日志 | 整数，单位秒，默认为7*24*3600，即7天 |
| drop_delay_log | 对于超过 `max_log_delay_time` 日志的处理策略 | 0 不丢弃，把日志时间修改为当前时间; 1 丢弃，默认为 1 （丢弃）|
| persistent | 断点续传功能，每次发送前会把日志保存到本地的binlog文件，只有发送成功才会删除，保证日志上传At Least Once。注意：持久化功能会耗费较多资源，请谨慎开启；该选项打开后，发送线程数必须设置为1，大数据量场景不建议使用。 |1 开启断点续传功能， 0 关闭，默认0（关闭） |
| persistent_file_path | 持久化的文件名，需要保证文件所在的文件夹已创建。配置多个客户端时，不应设置相同文件 | 字符串，默认为空 |
| persistent_force_flush | 是否每次AddLog强制刷新，高可靠性场景建议打开 | 0 关闭，1 开启，默认为0（关闭）|
| persistent_max_file_count | 持久化文件滚动个数，建议设置成10 | 整数，默认为0 | 
| persistent_max_file_size | 每个持久化文件的大小，建议设置成1-10M | 整数，单位为字节，默认为 0 | 
| persistent_max_log_count | 本地最多缓存的日志数，不建议超过1M，通常设置为65536即可 | 整数，默认为 0 |
| drop_unauthorized_log | 是否丢弃鉴权失败的日志，0 不丢弃，1丢弃 | 整数，默认为 0，即不丢弃 |


### 样例代码
```c
#include "log_api.h"
#include "log_producer_config.h"
#include "log_producer_client.h"

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

    // set send thread count
    log_producer_config_set_send_thread_count(config, 4);

    return create_log_producer(config, on_send_done);
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
```