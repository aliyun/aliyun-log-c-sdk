# API-Key 鉴权使用说明

API-Key 鉴权模式允许用户使用 API-Key 进行身份认证，无需 AccessKey ID / AccessKey Secret，无需签名计算。SDK 会在 Authorization Header 中携带 `Bearer <api-key>` 进行认证。

## 约束

- **HTTPS 强制**：API-Key 模式**必须**使用 HTTPS，否则 producer 创建会失败
- **与 AK 模式互斥**：不能同时设置 API-Key 和 AccessKey ID / AccessKey Secret，否则 producer 创建会失败
- **不支持轮转**：API-Key 模式不支持动态凭证轮转
- **持久化兼容**：API-Key 模式与持久化存储功能完全兼容，凭证不会持久化到磁盘

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

// 3. 可选：启用持久化存储
log_producer_config_set_persistent(config, 1);
log_producer_config_set_persistent_file_path(config, "/app/data/log.dat");
log_producer_config_set_persistent_max_file_count(config, 10);
log_producer_config_set_persistent_max_file_size(config, 1024 * 1024);
log_producer_config_set_persistent_max_log_count(config, 65536);

// 4. 创建 producer 并使用
log_producer * producer = create_log_producer(config, NULL);
log_producer_client * client = get_log_producer_client(producer, NULL);

// 发送日志
log_producer_client_add_log(client, 4, "key1", "value1", "key2", "value2");

// 5. 清理
destroy_log_producer(producer);
```

## 常见错误

| 错误信息 | 原因 | 解决方法 |
|---------|------|---------|
| `api-key mode requires HTTPS` | 未使用 HTTPS | endpoint 以 `https://` 开头，或调用 `log_producer_config_set_using_http(config, 1)` |
| `api-key mode is mutually exclusive with access-key mode` | 同时设置了 API-Key 和 AccessKey | 只使用其中一种鉴权方式 |
| `api-key mode requires a non-empty api-key` | API-Key 为空 | 传入有效的 API-Key 字符串 |
