# Aliyun LOG SDK for C

## 安装方法
### 环境依赖
LOG C SDK使用curl进行网络操作，无论是作为客户端还是服务器端，都需要依赖curl。
另外，LOG C SDK使用apr/apr-util库解决内存管理以及跨平台问题。
LOG C SDK并没有带上这几个外部库，您需要确认这些库已经安装，并且将它们的头文件目录和库文件目录都加入到了项目中。

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

#### 构造LogGroup：

```
log_group_builder* bder = log_group_create();

	add_source(bder,"mSource",sizeof("mSource"));
	add_topic(bder,"mTopic", sizeof("mTopic"));

    add_log(bder);
    	add_log_key_value(bder, "K11", strlen("K11"), V11, strlen("K11"));
    	add_log_key_value(bder, "K12", strlen("K12"), V12, strlen("K12"));
    	add_log_key_value(bder, "K13", strlen("K13"), V13, strlen("K13"));
    	
    add_log(bder);
    	add_log_key_value(bder, "K21", strlen("K21"), V21, strlen("K21"));
    	add_log_key_value(bder, "K22", strlen("K22"), V22, strlen("K22"));
    	add_log_key_value(bder, "K23", strlen("K23"), V23, strlen("K23"));

log_group_destroy(bder);
 	
```

#### 发送LogGroup

```
log_post_logs_from_proto_buf("LOG_ENDPOINT", "ACCESS_KEY_ID","ACCESS_KEY_SECRET","TOKEN", "PROJECT_NAME", "LOGSTORE_NAME", bder);

```

###如果不需要此SDk执行发送逻辑，则可以用一下接口，获得要发送的请求的内容：
#### 构造Http包内容

```
log_http_cont* cont =  log_create_http_cont("LOG_ENDPOINT", "ACCESS_KEY_ID","ACCESS_KEY_SECRET","TOKEN", "PROJECT_NAME", "LOGSTORE_NAME", bder);

```

`log_http_cont`包括：

 - Url(char\*)
 - Header(apr_table)
 - Body({void\* buf,size_t length})
 
注意：log_http_cont 和 log_group_builder 共用一个内存池，所以构造`log_http_cont`后，调用`log_group_destroy`或`log_clean_http_cont`中任一个都将同时销毁`builder`和`http_cont`.

## 联系我们
- [阿里云LOG官方网站](https://www.aliyun.com/product/sls/)
- [阿里云LOG官方论坛](https://yq.aliyun.com/groups/50)
- 阿里云官方技术支持：[提交工单](https://workorder.console.aliyun.com/#/ticket/createIndex)
