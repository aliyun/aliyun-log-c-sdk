 日志服务C Producer Bricks 是专为资源限制极强的IOT设备提供的日志上传解决方案。C Producer Bricks包装了实现日志上传的最小化依赖，编译后的库只有10KB。


## 功能特点
* 极致的代码体积与运行内存占用
    * 编译后库只有10KB，运行期间内存可控制在1KB以内
* 极致的日志序列化性能
    * 无依赖实现Proto Buffer，每秒可达1.7GB的序列化速度
* 可配置的附加功能
    * 可通过cmake选项开启LZ4压缩、日志字段迭代添加等功能
* 完全平台无关
    * 只依赖极少的C标准库，和特定平台无关

## 使用步骤

![image.png](http://ata2-img.cn-hangzhou.img-pub.aliyun-inc.com/a44e6ec0afcf74a8c19d41089b220218.png)

### 下载&编译代码
#### 下载Producer Bricks 代码
Producer Bricks代码在bricks分支，您可以使用如下命令获取代码：
```shell
git clone https://github.com/aliyun/aliyun-log-c-sdk.git -b bricks
```

#### 环境依赖
C Producer Bricks 默认不依赖于任何第三方库，如您需要编译示例程序，需要安装libcurl库。

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

#### C Producer Bricks 安装

  安装时请在cmake命令中指定第三方库头文件以及库文件的路径，典型的编译命令如下：
```shell
    cmake .
    make
    make install
```


* 注意：
 - 默认会编译示例程序，使用 `-DWITH_SAMPLE=OFF` 关闭此选项
 - 默认关闭lz4压缩，如需使用lz4压缩，使用 `-DWITH_LZ4=ON` 打开此选项（日志通常会压缩5-10倍，打开后编译体积会增加10KB左右）
 - 默认不生成逐个添加日志字段的功能，如有需求，使用 `-DADD_LOG_KEY_VALUE_FUN=ON` 打开此选项
 - 执行`cmake .` 时默认会到`/usr/local/`下面去寻找curl的头文件和库文件。
 - 默认编译是Debug类型，可以指定以下几种编译类型： Debug, Release, RelWithDebInfo和MinSizeRel，如果要使用release类型编译，则执行cmake . -DCMAKE_BUILD_TYPE=Release
 - 如果您在安装curl时指定了安装目录，则需要在执行cmake时指定这些库的路径，比如：
```shell
   cmake . -DCURL_INCLUDE_DIR=/usr/local/include/curl/ -DCURL_LIBRARY=/usr/local/lib/libcurl.a
```
 - 如果要指定安装目录，则需要在cmake时增加： `-DCMAKE_INSTALL_PREFIX=/your/install/path/usr/local/`

### 适配log_util接口
您需要按照`src/log_util.h`中定义的接口形式实现以下3个接口：

```c

typedef void (*md5_to_string_fun)(const char * buffer, int bufLen, char * md5);

typedef int (*hmac_sha1_signature_to_base64_fun)(const char * sig, int sigLen, const char * key, int keyLen, char * base64);

typedef void (*get_now_time_str_fun)(char * buffer);

extern md5_to_string_fun g_log_md5_to_string_fun;
extern hmac_sha1_signature_to_base64_fun g_log_hmac_sha1_signature_to_base64_fun;
extern get_now_time_str_fun g_log_get_now_time_str_fun;
```

* 接口详细定义参见`src/log_util.h`
* 示例参见`sample/log_util_imp.h`以及`sample/log_util_imp.c`

### 实现http发送
您需要根据平台接口实现打包好后的http数据发送，日志打包参见以下接口：

```c

post_log_header pack_logs_from_buffer_lz4(const char *endpoint, const char * accesskeyId, const char *accessKey,
                                      const char *project, const char *logstore,
                                      const char * data, uint32_t data_size, uint32_t raw_data_size);

post_log_header pack_logs_from_raw_buffer(const char *endpoint, const char * accesskeyId, const char *accessKey,
                                      const char *project, const char *logstore,
                                      const char * data, uint32_t raw_data_size);
                                      
void free_post_log_header(post_log_header header);
```

* 接口详细定义参见`src/log_api.h`
* 示例参见`sample/log_post_logs_sample.c`


## 示例

我们基于libcurl实现了日志上传的基本功能，详情请参见sample目录中的相关代码。 



