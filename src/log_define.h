#ifndef LIBLOG_DEFINE_H
#define LIBLOG_DEFINE_H


#ifdef _WIN32
#define LOG_EXPORT _declspec(dllexport)
#else
#define LOG_EXPORT
#endif

#ifdef _WIN32
#define LOG_EXPORT_API __declspec(dllexport)
#else
#define LOG_EXPORT_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
# define LOG_CPP_START extern "C" {
# define LOG_CPP_END }
#else
# define LOG_CPP_START
# define LOG_CPP_END
#endif

typedef int log_status_t;

struct _post_log_result
{
    int statusCode;
    char * errorMessage;
    char * requestID;
};

typedef struct _post_log_result post_log_result;

enum _auth_version
{
    AUTH_VERSION_1 = 1,
    AUTH_VERSION_APIKEY   // API-Key Bearer token auth
};

typedef enum _auth_version auth_version;

#endif
