# API-Key 鉴权使用说明

API-Key 模式使用 Authorization Header 中的 `Bearer <api-key>` 进行鉴权，不再使用 AccessKey 签名。

## 约束

- 必须使用 HTTPS。
- API-Key 模式与 AccessKey 模式互斥。
- producer 创建前使用 `log_producer_config_set_api_key()` 设置初始 API-Key。
- producer 运行期间使用 `log_producer_config_reset_api_key()` 线程安全地轮转 API-Key。

## 使用方式

```c
log_producer_config * config = create_log_producer_config();
log_producer_config_set_endpoint(config, "https://cn-hangzhou.log.aliyuncs.com");
log_producer_config_set_project(config, "my-project");
log_producer_config_set_logstore(config, "my-logstore");
log_producer_config_set_api_key(config, "your-api-key");

log_producer * producer = create_log_producer(config, NULL, NULL);

// 正在发送的请求继续使用旧快照，后续请求使用新的 API-Key。
log_producer_config_reset_api_key(config, "your-new-api-key");

destroy_log_producer(producer);
```
