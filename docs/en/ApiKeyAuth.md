# API-Key Authentication Guide

API-Key mode sends `Bearer <api-key>` in the Authorization header instead of using AccessKey signing.

## Constraints

- HTTPS is required.
- API-Key mode is mutually exclusive with AccessKey mode.
- Use `log_producer_config_set_api_key()` before creating the producer.
- Use `log_producer_config_reset_api_key()` for thread-safe runtime rotation.

## Usage

```c
log_producer_config * config = create_log_producer_config();
log_producer_config_set_endpoint(config, "https://cn-hangzhou.log.aliyuncs.com");
log_producer_config_set_project(config, "my-project");
log_producer_config_set_logstore(config, "my-logstore");
log_producer_config_set_api_key(config, "your-api-key");

log_producer * producer = create_log_producer(config, NULL, NULL);

// In-flight requests keep their old snapshot; subsequent requests use the new key.
log_producer_config_reset_api_key(config, "your-new-api-key");

destroy_log_producer(producer);
```
