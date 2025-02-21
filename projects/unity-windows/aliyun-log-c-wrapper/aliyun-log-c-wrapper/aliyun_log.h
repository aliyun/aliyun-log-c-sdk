#pragma once
#include <stdint.h>

#ifndef ALIYUN_LOG_EXPORT
#define ALIYUN_LOG_EXPORT __declspec(dllexport)
#endif // !ALIYUN_LOG_EXPORT

ALIYUN_LOG_EXPORT struct AliyunLog {
	void* config;
	void* client;
	void* producer;
};

ALIYUN_LOG_EXPORT typedef void(*aliyun_log_send_done_function)(const char* config_name, uint32_t result, size_t log_bytes, size_t compressed_bytes, const char* error_message, void* user_param);


extern "C" ALIYUN_LOG_EXPORT AliyunLog * aliyun_log_create(
	const char* endpoint,
	const char* project,
	const char* logstore,
	const char* access_key_id,
	const char* access_key_secret,
	const char* access_key_token
);

extern "C" ALIYUN_LOG_EXPORT void aliyun_log_destroy(AliyunLog * aliyun_log);

extern "C" ALIYUN_LOG_EXPORT void aliyun_log_set_send_done_function(aliyun_log_send_done_function send_done_function);

extern "C" ALIYUN_LOG_EXPORT uint32_t aliyun_log_add_log(AliyunLog * aliyun_log, uint32_t length, char** key_values);

extern "C" ALIYUN_LOG_EXPORT void aliyun_log_set_accesskey(AliyunLog * aliyun_log, const char* access_key_id, const char* access_key_secret, const char* access_key_token);

extern "C" ALIYUN_LOG_EXPORT void aliyun_log_set_endpoint(AliyunLog * aliyun_log, const char* endpoint);

extern "C" ALIYUN_LOG_EXPORT void aliyun_log_set_project(AliyunLog * aliyun_log, const char* project);

extern "C" ALIYUN_LOG_EXPORT void aliyun_log_set_logstore(AliyunLog * aliyun_log, const char* logstore);