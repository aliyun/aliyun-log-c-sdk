//
// Created by ZhangCheng on 21/11/2017.
//

#include "log_producer_common.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

log_producer_result LOG_PRODUCER_OK = 0;
log_producer_result LOG_PRODUCER_INVALID = 1;
log_producer_result LOG_PRODUCER_WRITE_ERROR = 2;
log_producer_result LOG_PRODUCER_DROP_ERROR = 3;
log_producer_result LOG_PRODUCER_SEND_NETWORK_ERROR = 4;
log_producer_result LOG_PRODUCER_SEND_QUOTA_ERROR = 5;
log_producer_result LOG_PRODUCER_SEND_UNAUTHORIZED = 6;
log_producer_result LOG_PRODUCER_SEND_SERVER_ERROR = 7;
log_producer_result LOG_PRODUCER_SEND_DISCARD_ERROR = 8;
log_producer_result LOG_PRODUCER_SEND_TIME_ERROR = 9;

const char * log_producer_result_string[] = {
        "LOG_PRODUCER_OK",
        "LOG_PRODUCER_INVALID",
        "LOG_PRODUCER_WRITE_ERROR",
        "LOG_PRODUCER_DROP_ERROR",
        "LOG_PRODUCER_SEND_NETWORK_ERROR",
        "LOG_PRODUCER_SEND_QUOTA_ERROR",
        "LOG_PRODUCER_SEND_UNAUTHORIZED",
        "LOG_PRODUCER_SEND_SERVER_ERROR",
        "LOG_PRODUCER_SEND_DISCARD_ERROR",
        "LOG_PRODUCER_SEND_TIME_ERROR"
};

const char * log_producer_level_string[] = {
        "LOG_PRODUCER_LEVEL_NONE",
        "LOG_PRODUCER_LEVEL_DEBUG",
        "LOG_PRODUCER_LEVEL_INFO",
        "LOG_PRODUCER_LEVEL_WARN",
        "LOG_PRODUCER_LEVEL_ERROR",
        "LOG_PRODUCER_LEVEL_FATAL",
        "LOG_PRODUCER_LEVEL_MAX"
};

int is_log_producer_result_ok(log_producer_result rst)
{
    return rst == LOG_PRODUCER_OK;
}

const char * get_log_producer_result_string(log_producer_result rst)
{
    if ((int)rst < 0 || (int)rst >= sizeof(log_producer_result_string) / sizeof(char *))
    {
        return log_producer_result_string[1];
    }
    return log_producer_result_string[rst];
}

char * get_ip_by_host_name(apr_pool_t * pool)
{
    char hostname[1024];
    gethostname(hostname, 1024);

    struct hostent* entry = gethostbyname(hostname);
    if (entry == NULL)
    {
        return NULL;
    }
    struct in_addr* addr = (struct in_addr*)entry -> h_addr_list[0];
    if (addr == NULL)
    {
        return NULL;
    }
    char* ip = inet_ntoa(*addr);
    char * ip_addr = (char *)apr_pcalloc(pool, 64);
    strcpy(ip_addr, ip);
    return ip_addr;
}

void log_producer_atoi(char * buffer, int32_t value)
{
    char tmp[12];
    char* tp = tmp;
    int i;
    unsigned v;
    char* sp;
    int sign = value < 0;
    if (sign)
        v = -value;
    else
        v = (unsigned)value;
    while (v || tp == tmp)
    {
        i = v % 10;
        v = v / 10;
        *tp++ = i+'0';
    }
    sp = buffer;
    if (sign)
        *sp++ = '-';
    while (tp > tmp)
        *sp++ = *--tp;
    *sp = 0;
    return;
}
