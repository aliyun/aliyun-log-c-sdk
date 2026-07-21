# API-Key 鉴权使用说明

API-Key 鉴权模式允许用户使用 API-Key 进行身份认证，无需 AccessKey ID / AccessKey Secret，无需签名计算。SDK 会在 Authorization Header 中携带 `Bearer <api-key>` 进行认证。

## 约束

- **HTTPS 强制**：API-Key 模式**必须**使用 HTTPS，否则 producer 创建会失败
- **与 AK 模式互斥**：不能同时设置 API-Key 和 AccessKey ID / AccessKey Secret，否则 producer 创建会失败
- **与动态凭证互斥**：不能同时设置 API-Key 和 credentials callback
- **线程安全轮转**：producer 启动后调用 `log_producer_config_reset_api_key()` 更新 API-Key

## 使用方式

```c
#include "log_api.h"
#include "log_producer_config.h"
#include "log_producer_client.h"

// 1. 创建 producer 配置
log_producer_config * config = create_log_producer_config();

// endpoint 必须以 "https://" 开头
log_producer_config_set_endpoint(config, "https://cn-hangzhou.log.aliyuncs.com");
log_producer_config_set_project(config, "my-project");
log_producer_config_set_logstore(config, "my-logstore");

// 2. 设置 API-Key（会自动设置 authVersion = AUTH_VERSION_APIKEY）
//    注意：不要再调用 log_producer_config_set_access_id / set_access_key
log_producer_config_set_api_key(config, "your-api-key");

// 3. 创建 producer 并使用
log_producer * producer = create_log_producer(config, NULL);
log_producer_client * client = get_log_producer_client(producer, NULL);

// 发送日志
log_producer_client_add_log(client, 4, "key1", "value1", "key2", "value2");

// producer 运行期间线程安全地轮转 API-Key。正在发送的请求继续使用旧快照，
// 后续请求使用新的 API-Key。
log_producer_config_reset_api_key(config, "your-new-api-key");

// 4. 清理
destroy_log_producer(producer);
```

完整示例可参见 [log_apikey_sample.c](../../sample/log_apikey_sample.c)

## 常见错误

| 错误信息 | 原因 | 解决方法 |
|---------|------|---------|
| `api-key mode requires HTTPS` | 未使用 HTTPS | endpoint 以 `https://` 开头，或调用 `log_producer_config_set_using_http(config, 1)` |
| `api-key mode is mutually exclusive with access-key mode` | 同时设置了 API-Key 和 AccessKey | 只使用其中一种鉴权方式 |
| `api-key mode does not support credentials callback` | 同时设置了 API-Key 和动态凭证回调 | API-Key 模式下不要设置 credentials callback |
| `api-key mode requires a non-empty api-key` | API-Key 为空 | 传入有效的 API-Key 字符串 |
