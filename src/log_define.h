#ifndef LIBLOG_DEFINE_H
#define LIBLOG_DEFINE_H


#ifdef WIN32
#define LOG_EXPORT _declspec(dllexport)
#else
#define LOG_EXPORT
#endif

#ifdef __cplusplus
# define LOG_CPP_START extern "C" {
# define LOG_CPP_END }
#else
# define LOG_CPP_START
# define LOG_CPP_END
#endif

#define LOGE_SERVER_BUSY "ServerBusy"
#define LOGE_INTERNAL_SERVER_ERROR "InternalServerError"
#define LOGE_UNAUTHORIZED "Unauthorized"
#define LOGE_WRITE_QUOTA_EXCEED "WriteQuotaExceed"
#define LOGE_SHARD_WRITE_QUOTA_EXCEED "ShardWriteQuotaExceed"
#define LOGE_TIME_EXPIRED "RequestTimeExpired"

#endif//LIBLOG_DEFINE_H
