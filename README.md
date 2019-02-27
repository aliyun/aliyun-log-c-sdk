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
* 可靠退出：程序退出时，会调用接口将日志持久化，待下次应用启动时将数据发送，保证数据可靠性。详情参见[程序可靠退出方案](save_send_buffer.md)。

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
 - 默认编译是Release类型，可以指定以下几种编译类型： Debug, Release, RelWithDebInfo和MinSizeRel，如果要使用Debug类型编译，则执行cmake . -DCMAKE_BUILD_TYPE=Debug
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
| connect_timeout_sec | 网络连接超时时间 | 整数，单位秒，默认为10 |
| send_timeout_sec | 日志发送超时时间 | 整数，单位秒，默认为15 |
| destroy_flusher_wait_sec | flusher线程销毁最大等待时间 | 整数，单位秒，默认为1 |
| destroy_sender_wait_sec | sender线程池销毁最大等待时间 | 整数，单位秒，默认为1 |

### 相关限制
* C Producer销毁时，会尽可能将缓存中的数据发送出去，若您不对未发送的数据进行处理，则有一定概率丢失数据。处理方法参见[程序可靠退出方案](save_send_buffer.md)
* C Producer销毁的最长时间可能为 `send_timeout_sec` + `destroy_flusher_wait_sec` + `destroy_sender_wait_sec`
### 样例代码

请参考`sample`目录中的`log_producer_sample.c`。