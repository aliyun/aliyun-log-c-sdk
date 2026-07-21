#include "log_producer_config.h"
#include "sds.h"

#include <assert.h>
#include <pthread.h>
#include <string.h>

#define ROTATION_COUNT 10000
#define READER_COUNT 4

static const char * API_KEY_INITIAL = "api-key-initial-00000000000000000000";
static const char * API_KEY_A = "api-key-a-aaaaaaaaaaaaaaaaaaaaaaaa";
static const char * API_KEY_B = "api-key-b-bbbbbbbbbbbbbbbbbbbbbbbb";

typedef struct _rotation_context
{
    log_producer_config * config;
} rotation_context;

static int is_expected_key(const char * api_key)
{
    return strcmp(api_key, API_KEY_INITIAL) == 0
        || strcmp(api_key, API_KEY_A) == 0
        || strcmp(api_key, API_KEY_B) == 0;
}

static void * rotate_api_key(void * userdata)
{
    rotation_context * context = (rotation_context *)userdata;
    int i = 0;
    for (; i < ROTATION_COUNT; ++i)
    {
        log_producer_config_reset_api_key(context->config, i % 2 == 0 ? API_KEY_A : API_KEY_B);
    }
    return NULL;
}

static void * read_api_key(void * userdata)
{
    rotation_context * context = (rotation_context *)userdata;
    int i = 0;
    for (; i < ROTATION_COUNT; ++i)
    {
        sds api_key = NULL;
        log_producer_config_get_api_key(context->config, &api_key);
        if (api_key == NULL || !is_expected_key(api_key))
        {
            sdsfree(api_key);
            return (void *)1;
        }
        sdsfree(api_key);
    }
    return NULL;
}

int main(void)
{
    log_producer_config * static_ak_config = create_log_producer_config();
    log_producer_config * config = create_log_producer_config();
    rotation_context context;
    sds access_key_id = NULL;
    sds access_key_secret = NULL;
    sds security_token = NULL;
    sds in_flight_snapshot = NULL;
    sds rotated_snapshot = NULL;
    pthread_t writer;
    pthread_t readers[READER_COUNT];
    int i = 0;

    assert(static_ak_config != NULL);
    assert(static_ak_config->securityTokenLock == NULL);
    log_producer_config_set_access_id(static_ak_config, "test-access-key-id");
    log_producer_config_set_access_key(static_ak_config, "test-access-key-secret");
    log_producer_config_get_security(static_ak_config, &access_key_id, &access_key_secret, &security_token);
    assert(static_ak_config->securityTokenLock == NULL);
    assert(strcmp(access_key_id, "test-access-key-id") == 0);
    assert(strcmp(access_key_secret, "test-access-key-secret") == 0);
    sdsfree(access_key_id);
    sdsfree(access_key_secret);
    sdsfree(security_token);
    destroy_log_producer_config(static_ak_config);

    assert(config != NULL);
    assert(config->securityTokenLock == NULL);
    log_producer_config_set_api_key(config, API_KEY_INITIAL);
    assert(config->securityTokenLock != NULL);
    log_producer_config_get_api_key(config, &in_flight_snapshot);
    log_producer_config_reset_api_key(config, API_KEY_A);
    log_producer_config_get_api_key(config, &rotated_snapshot);
    assert(strcmp(in_flight_snapshot, API_KEY_INITIAL) == 0);
    assert(strcmp(rotated_snapshot, API_KEY_A) == 0);
    sdsfree(in_flight_snapshot);
    sdsfree(rotated_snapshot);
    context.config = config;

    assert(pthread_create(&writer, NULL, rotate_api_key, &context) == 0);
    for (; i < READER_COUNT; ++i)
    {
        assert(pthread_create(&readers[i], NULL, read_api_key, &context) == 0);
    }

    assert(pthread_join(writer, NULL) == 0);
    for (i = 0; i < READER_COUNT; ++i)
    {
        void * result = NULL;
        assert(pthread_join(readers[i], &result) == 0);
        assert(result == NULL);
    }

    destroy_log_producer_config(config);
    return 0;
}
