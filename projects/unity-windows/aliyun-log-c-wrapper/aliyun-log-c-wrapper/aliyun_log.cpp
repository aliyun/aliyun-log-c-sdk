#include "aliyun_log.h"
#include "log_producer_config.h"
#include "log_producer_client.h"


static aliyun_log_send_done_function _send_done_function;

void internal_on_log_send_done(const char* config_name, log_producer_result result, size_t log_bytes, size_t compressed_bytes, const char* req_id, const char* message, const unsigned char* raw_buffer, void* user_param)
{
	if (NULL == user_param)
	{
		return;
	}

	if (NULL == _send_done_function)
	{
		return;
	}

	_send_done_function(config_name, result, log_bytes, compressed_bytes, message, user_param);
}

AliyunLog* aliyun_log_create(
	const char* endpoint,
	const char* project,
	const char* logstore,
	const char* access_key_id,
	const char* access_key_secret,
	const char* access_key_token)
{
	if (log_producer_env_init() != LOG_PRODUCER_OK)
	{
		return NULL;
	}

	log_producer_config* config = create_log_producer_config();
	log_producer_config_set_source(config, "Windows");
	log_producer_config_set_send_thread_count(config, 1);

	log_producer_config_set_endpoint(config, endpoint);
	log_producer_config_set_project(config, project);
	log_producer_config_set_logstore(config, logstore);
	if (NULL != access_key_token)
	{
		log_producer_config_reset_security_token(config, access_key_id, access_key_secret, access_key_token);
	}
	else
	{
		log_producer_config_set_access_id(config, access_key_id);
		log_producer_config_set_access_key(config, access_key_secret);
	}
	

	log_producer_config_set_packet_log_bytes(config, 4 * 1024 * 1024);
	log_producer_config_set_packet_log_count(config, 4096);
	log_producer_config_set_packet_timeout(config, 3000);
	log_producer_config_set_max_buffer_limit(config, 32 * 1024 * 1024);

	// async_fetch_server_time_from_sls(config);

	AliyunLog* aliyun_log = (AliyunLog*)malloc(sizeof(AliyunLog));

	log_producer* producer = create_log_producer(config, internal_on_log_send_done, (void*)aliyun_log);

	log_producer_client* client = get_log_producer_client(producer, NULL);


	
	aliyun_log->config = config;
	aliyun_log->client = client;
	aliyun_log->producer = producer;


	return aliyun_log;
}

uint32_t aliyun_log_add_log(AliyunLog* aliyun_log, uint32_t length, char** key_values)
{
	if (NULL == aliyun_log)
	{
		return LOG_PRODUCER_INVALID;
	}

	char** keys = (char**)malloc(sizeof(char*) * length);
	size_t* keys_length = (size_t*)malloc(sizeof(size_t) * length);
	char** values = (char**)malloc(sizeof(char*) * length);
	size_t* values_length = (size_t*)malloc(sizeof(size_t) * length);
	for (int i = 0; i < length; i++)
	{
		keys[i] = key_values[2 * i];
		keys_length[i] = strlen(keys[i]);

		values[i] = key_values[2 * i + 1];
		values_length[i] = strlen(values[i]);
	}

	log_producer_result result = log_producer_client_add_log_with_len((log_producer_client*)aliyun_log->client, length, keys, keys_length, values, values_length, 0);

	free(keys);
	free(keys_length);
	free(values);
	free(values_length);

	return result;
}

void aliyun_log_set_send_done_function(aliyun_log_send_done_function send_done_function)
{
	_send_done_function = send_done_function;
}

void aliyun_log_destroy(AliyunLog* aliyun_log)
{
	if (NULL == aliyun_log || NULL == aliyun_log->producer)
	{
		return;
	}

	destroy_log_producer((log_producer*)aliyun_log->producer);
	free(aliyun_log);
}


void aliyun_log_set_accesskey(AliyunLog * aliyun_log, const char* access_key_id, const char* access_key_secret, const char* access_key_token)
{
	if (NULL == aliyun_log)
	{
		return;
	}

	if (NULL == access_key_id || NULL == access_key_secret)
	{
		return;
	}

	if (NULL != access_key_token && 0 != strlen(access_key_token))
	{
		log_producer_config_reset_security_token((log_producer_config*)aliyun_log->config, access_key_id, access_key_secret, access_key_token);
	}
	else
	{
		log_producer_config_set_access_id((log_producer_config*)aliyun_log->config, access_key_id);
		log_producer_config_set_access_key((log_producer_config*)aliyun_log->config, access_key_secret);
	}
}


void aliyun_log_set_endpoint(AliyunLog* aliyun_log, const char* endpoint)
{
	if (NULL == aliyun_log || NULL == (log_producer_config*)aliyun_log->config)
	{
		return;
	}

	log_producer_config_set_endpoint((log_producer_config*)aliyun_log->config, endpoint);
}

void aliyun_log_set_project(AliyunLog* aliyun_log, const char* project)
{
	if (NULL == aliyun_log || NULL == (log_producer_config*)aliyun_log->config)
	{
		return;
	}

	log_producer_config_set_project((log_producer_config*)aliyun_log->config, project);
}

void aliyun_log_set_logstore(AliyunLog* aliyun_log, const char* logstore)
{
	if (NULL == aliyun_log || NULL == (log_producer_config*)aliyun_log->config)
	{
		return;
	}

	log_producer_config_set_logstore((log_producer_config*)aliyun_log->config, logstore);
}