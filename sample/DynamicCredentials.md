# 动态凭证使用说明

动态凭证功能允许用户传入一个自定义的获取凭证的回调函数，实现临时 ak 的动态轮转能力，提升安全性。

## 1. 自定义凭证获取回调函数

```c
int get_credentials_callback(log_producer_credentials * credentials, void * userdata)
{
    // 从STS或其他服务获取临时凭证
    const char* ak_id = "your_access_key_id";
    const char* ak_secret = "your_access_key_secret";
    const char* token = "your_security_token";  // 可选
    int64_t expire_ts = time(NULL) + 3600;  // 过期时间，unix timestamp, 单位为秒
    
    // 设置凭证，ak 字符串的生命周期由用户自行维护， sls sdk 只会拷贝字符串
    return log_producer_credentials_set(
        credentials,
        ak_id, strlen(ak_id),
        ak_secret, strlen(ak_secret),
        token, token ? strlen(token) : 0,
        expire_ts
    );
}

```

## 2. 创建 producer 配置，并设置回调函数

```c
// 创建 producer 配置
log_producer_config * config = create_log_producer_config();
log_producer_config_set_endpoint(config, "cn-hangzhou.log.aliyuncs.com");
log_producer_config_set_project(config, "my-project");
log_producer_config_set_logstore(config, "my-logstore");

// 设置回调函数和用户数据
log_producer_config_set_credentials_callback(config, get_credentials_callback);
// 可选，可设置一个 userdata，在回调时传入。userdata 的生命周期必须长于 producer，在 producer destroy 之前必须有效
log_producer_config_set_credentials_userdata(config, &my_userdata);

// 3. 创建producer并使用
log_producer * producer = create_log_producer(config, NULL);
log_producer_client * client = get_log_producer_client(producer, NULL);

// 发送日志
log_producer_client_add_log(client, 4, "key1", "value1", "key2", "value2");

// 4. 清理
destroy_log_producer(producer);
```
完整示例可参见 [log_dynamic_credentials_sample.c](log_dynamic_credentials_sample.c)

## 回调触发时机

producer 会在首次发送日志，或者凭证将要过期时（提前1-2分钟）触发回调函数。
