# API-Key Authentication Guide

The API-Key authentication mode allows users to authenticate using an API-Key without AccessKey ID / AccessKey Secret and without signature calculation. The SDK sends `Bearer <api-key>` in the Authorization header for authentication.

## Constraints

- **HTTPS Required**: API-Key mode **must** use HTTPS; otherwise producer creation will fail
- **Mutually Exclusive with AK Mode**: Cannot set both API-Key and AccessKey ID / AccessKey Secret simultaneously; otherwise producer creation will fail
- **No Rotation Support**: API-Key mode does not support dynamic credential rotation
- **Persistent Compatible**: API-Key mode is fully compatible with persistent storage; credentials are never persisted to disk

## Usage

```c
#include "log_api.h"
#include "log_producer_config.h"
#include "log_producer_client.h"

// 1. Create producer config
log_producer_config * config = create_log_producer_config();

// Endpoint must start with "https://"
log_producer_config_set_endpoint(config, "https://cn-hangzhou.log.aliyuncs.com");
log_producer_config_set_project(config, "my-project");
log_producer_config_set_logstore(config, "my-logstore");

// 2. Set API-Key (automatically sets authVersion = AUTH_VERSION_APIKEY)
//    Note: Do NOT call log_producer_config_set_access_id / set_access_key
log_producer_config_set_api_key(config, "your-api-key");

// 3. Optional: enable persistent storage
log_producer_config_set_persistent(config, 1);
log_producer_config_set_persistent_file_path(config, "/app/data/log.dat");
log_producer_config_set_persistent_max_file_count(config, 10);
log_producer_config_set_persistent_max_file_size(config, 1024 * 1024);
log_producer_config_set_persistent_max_log_count(config, 65536);

// 4. Create producer and use it
log_producer * producer = create_log_producer(config, NULL);
log_producer_client * client = get_log_producer_client(producer, NULL);

// Send logs
log_producer_client_add_log(client, 4, "key1", "value1", "key2", "value2");

// 5. Cleanup
destroy_log_producer(producer);
```

## Common Errors

| Error Message | Cause | Solution |
|--------------|-------|----------|
| `api-key mode requires HTTPS` | HTTPS not used | Ensure the endpoint starts with `https://`, or call `log_producer_config_set_using_http(config, 1)` |
| `api-key mode is mutually exclusive with access-key mode` | Both API-Key and AccessKey are set | Use only one authentication method |
| `api-key mode requires a non-empty api-key` | API-Key is empty | Provide a valid API-Key string |
