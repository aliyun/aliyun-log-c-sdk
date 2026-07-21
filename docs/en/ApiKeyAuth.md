# API-Key Authentication Guide

The API-Key authentication mode allows users to authenticate using an API-Key without AccessKey ID / AccessKey Secret and without signature calculation. The SDK sends `Bearer <api-key>` in the Authorization header for authentication.

## Constraints

- **HTTPS Required**: API-Key mode **must** use HTTPS; otherwise producer creation will fail
- **Mutually Exclusive with AK Mode**: Cannot set both API-Key and AccessKey ID / AccessKey Secret simultaneously; otherwise producer creation will fail
- **Mutually Exclusive with Dynamic Credentials**: Cannot set both API-Key and credentials callback
- **Thread-safe Rotation**: Use `log_producer_config_reset_api_key()` to rotate the key after the producer starts

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

// 3. Create producer and use it
log_producer * producer = create_log_producer(config, NULL);
log_producer_client * client = get_log_producer_client(producer, NULL);

// Send logs
log_producer_client_add_log(client, 4, "key1", "value1", "key2", "value2");

// Rotate the API-Key while the producer is running. In-flight requests keep
// using their request-scoped snapshot; subsequent requests use the new key.
log_producer_config_reset_api_key(config, "your-new-api-key");

// 4. Cleanup
destroy_log_producer(producer);
```

See [log_apikey_sample.c](../../sample/log_apikey_sample.c) for a complete example.

## Common Errors

| Error Message | Cause | Solution |
|--------------|-------|----------|
| `api-key mode requires HTTPS` | HTTPS not used | Ensure the endpoint starts with `https://`, or call `log_producer_config_set_using_http(config, 1)` |
| `api-key mode is mutually exclusive with access-key mode` | Both API-Key and AccessKey are set | Use only one authentication method |
| `api-key mode does not support credentials callback` | Both API-Key and credentials callback are set | Do not set credentials callback in API-Key mode |
| `api-key mode requires a non-empty api-key` | API-Key is empty | Provide a valid API-Key string |
