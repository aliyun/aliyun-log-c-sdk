 日志服务C Producer是用纯C编写的日志采集客户端，提供更加精简的环境依赖以及更低的资源占用，试用于各类嵌入式/智能设备的日志采集。
 
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

在以上场景中，C Producer 会简化您程序开发的步骤，您无需关心日志采集细节实现、也不用担心日志采集会影响您的业务正常运行，大大降低数据采集门槛。

# 与1.0版本（v1分支） C Producer对比
## 功能对比
相比1.0版本 C Producer，仅缺少以下功能：
1. 同一Producer只支持单个Client，不支持多Client设置
2. 不支持发送优先级设置
3. 不支持JSON方式的配置文件
4. 不提供日志相关宏定义包装
5. 不支持本地调试功能

## 性能对比
性能相对1.0版本 C Producer有30%以上提升。

## 资源占用对比
资源占用相比1.0版本 C Producer大大降低（详细请参考[性能测试](performance.md)）：
* 除去libcurl占用，运行期间内存占用80KB左右。
* 编译后的动态库仅有65KB（strip后）

# 分支选择
C Producer根据不同的设备类型和使用场景做了非常多的定制工作，因此分为几个分支来支持不同的场景，请根据实际需求选择。

|分支|状态|功能优势|建议使用场景|
|--|--|--|--|
| master | 可用 | 原lite分支，相比v1（1.0f）版本依赖、资源占用、性能等有大幅度提升，是目前SLS性能最强的Producer，推荐使用 | Linux服务器、嵌入式Linux |
| live | 可用 | 主要功能和master版本一致，增加最多平台的编译支持，包括Windows、Mac、Android、IOS等 | 非master支持的环境 |
| bricks | 可用 | 极致精简版本，binary和内存占用极低，但是功能非常弱，建议在资源非常受限的场景中使用 | 资源占用在10KB以内的场景，例如RTOS |
| persistent | 可用 | 相比master增加本地缓存功能，目前用于Android、IOS移动端版本的Native实现，本地缓存功能开启后只能单线程发送，不建议服务端使用 | 建议直接使用Android、IOS官方SDK |


* 注意：除以上分支外，不建议使用其他分支

# 安装方法
### 下载Producer代码
您可以使用如下命令获取代码：
```shell
git clone https://github.com/aliyun/aliyun-log-c-sdk.git
```

### 环境依赖
需要安装openssl
C Producer使用curl进行网络操作，您需要确认这些库已经安装，并且将它们的头文件目录和库文件目录都加入到了项目中。

#### libcurl下载以及安装

  libcurl建议 7.49.0 及以上版本

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
| log_queue_size | 缓存队列的最大长度，默认通过 max_buffer_limit/packet_log_bytes + 10 自动计算，可以手动设置以获得更大的队列大小。                                           | 整数形式，支持32-128000。默认值max_buffer_limit/packet_log_bytes + 10。                  |
| send_thread_count    | 指定发送线程数量，用于发送数据到日志服务。 | 整数形式，默认0，为0时发送与内部Flush公用一个线程，若您每秒日志发送不超过100条，无需设置此参数。                              |
| net_interface | 网络发送绑定的网卡名 | 字符串，为空时表示自动选择可用网卡 |
| connect_timeout_sec | 网络连接超时时间 | 整数，单位秒，默认为10 |
| send_timeout_sec | 日志发送超时时间 | 整数，单位秒，默认为15 |
| destroy_flusher_wait_sec | flusher线程销毁最大等待时间 | 整数，单位秒，默认为1 |
| destroy_sender_wait_sec | sender线程池销毁最大等待时间 | 整数，单位秒，默认为1 |
| compress_type | 压缩类型 | 枚举值，默认为lz4。 |
### 相关限制
* C Producer销毁时，会尽可能将缓存中的数据发送出去，若您不对未发送的数据进行处理，则有一定概率丢失数据。处理方法参见[程序可靠退出方案](save_send_buffer.md)
* C Producer销毁的最长时间可能为 `send_timeout_sec` + `destroy_flusher_wait_sec` + `destroy_sender_wait_sec`
### 样例代码

请参考`sample`目录中的`log_producer_sample.c`。



## C-Producer系列介绍

![image.png](http://ata2-img.cn-hangzhou.img-pub.aliyun-inc.com/bb6a86424ebba0efeca634dc2cc2ffd3.png)

> 关于C Producer Library的更多内容参见目录：[https://yq.aliyun.com/articles/304602](https://yq.aliyun.com/articles/304602)
目前针对不同的环境（例如网络服务器、ARM设备、以及RTOS等设备）从大到小我们提供了3种方案：


![image.png | left | 827x368](http://ata2-img.cn-hangzhou.img-pub.aliyun-inc.com/22e93dfe553b35a0d3c609840aa2db02.png "")

同时对于Producer我们进行了一系列的性能和资源优化，确保数据采集可以“塞”到任何IOT设备上，其中C Producer Bricks版本更是达到了极致的内存占用（库体积13KB，运行内存4KB以内）。


![image.png | left | 827x171](http://ata2-img.cn-hangzhou.img-pub.aliyun-inc.com/3371df540fcd57c18e943d3e44f30892.png "")


使用C Producer系列的客户有： 百万日活的天猫精灵、小朋友们最爱的故事机火火兔、 遍布全球的码牛、钉钉路由器、 兼容多平台的视频播放器、 实时传输帧图像的摄像头等。

这些智能SDK每天DAU超百万，遍布在全球各地的设备上，一天传输百TB数据。关于C Producer Library 的细节可以参考这篇文章: [智能设备日志利器：嵌入式日志客户端（C Producer）发布](https://yq.aliyun.com/articles/304602)。


![image.png | left | 827x264](http://ata2-img.cn-hangzhou.img-pub.aliyun-inc.com/470c6b49edd08278d42c4b8f836a6701.png "")


### 数据采集全球加速
IOT设备作为典型的“端”设备，通常都会部署在全国、甚至全球各地，部署区域的网络条件难以保证，这会对数据采集产生一个巨大的问题：数据采集受网络质量影响，可靠性难以保证。

针对以上问题，日志服务联合阿里云CDN推出了一款[全球数据上传自动加速](https://help.aliyun.com/document_detail/86817.html)方案：<span data-type="color" style="color:rgb(36, 41, 46)"><span data-type="background" style="background-color:rgb(255, 255, 255)">“基于阿里云CDN硬件资源，全球数据就近接入边缘节点，通过内部高速通道路由至LogHub，大大降低网络延迟和抖动 ”。</span></span>
该方案有如下特点：

* 全网边缘节点覆盖：全球1000+节点，国内700+节点，分布60多个国家和地区，覆盖六大洲
* 智能路由技术：实时探测网络质量，自动根据运营商、网络等状况选择最近接入
* 传输协议优化：CDN节点之间走私有协议、高效安全
* 使用便捷：只需1分钟即可开通加速服务，只需切换到专属加速域名即可获得加速效果

## ![image | left](http://ata2-img.cn-hangzhou.img-pub.aliyun-inc.com/e61177787e1c333103c9cd479c879070.png "image | left")

在我们的日志上传基准测试中，全球7个区域<span data-type="color" style="color:rgb(36, 41, 46)"><span data-type="background" style="background-color:rgb(255, 255, 255)">对比整体延时下降50%，在中东，欧洲、澳洲和新加坡等效果明显。除了平均延时下降外，整体稳定性也有较大提升（参见最下图，几乎没有任何抖动，而且超时请求基本为0）。确保无论在全球的何时何地，只要访问这个加速域名，就能够高效、便捷将数据采集到期望Region内。</span></span>

关于全球采集加速的更多内容，可<span data-type="color" style="color:rgb(36, 41, 46)"><span data-type="background" style="background-color:rgb(255, 255, 255)">参考我们的文章：</span></span>[数据采集新形态-全球加速](https://yq.aliyun.com/articles/620453)<span data-type="color" style="color:rgb(36, 41, 46)"><span data-type="background" style="background-color:rgb(255, 255, 255)"> 。</span></span>


![image.png | left | 827x396](http://ata2-img.cn-hangzhou.img-pub.aliyun-inc.com/a0b6820aa4a9b339edd9fa7615c1e13d.png "")



![image.png | left | 827x222](http://ata2-img.cn-hangzhou.img-pub.aliyun-inc.com/adec099e7475cd5e5d5ba7bca6e28b1e.png "")