# 底层日志发送接口
若producer提供的接口满足不了您的日志采集需求，您可以基于底层的日志发送接口，开发适合您的应用场景的日志采集API。
## 构造LogGroup：

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

## 发送LogGroup

```
log_post_logs_from_proto_buf("LOG_ENDPOINT", "ACCESS_KEY_ID","ACCESS_KEY_SECRET","TOKEN", "PROJECT_NAME", "LOGSTORE_NAME", bder);

```

### 手动构造Http包内容
 - 如果不需要此SDk执行发送逻辑，则可以用一下接口，获得要发送的请求的内容：
 

```
log_http_cont* cont =  log_create_http_cont("LOG_ENDPOINT", "ACCESS_KEY_ID","ACCESS_KEY_SECRET","TOKEN", "PROJECT_NAME", "LOGSTORE_NAME", bder);

```

`log_http_cont`包括：

 - `char* url`
 - `apr_table_t* header`
 - `void* body,size_t body_length`
 
注意：log_http_cont 和 log_group_builder 共用一个内存池，所以构造`log_http_cont`后，调用`log_group_destroy`或`log_clean_http_cont`中任一个都将同时销毁`builder`和`http_cont`.