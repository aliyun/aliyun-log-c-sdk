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
log_producer_result LOG_PRODUCER_SEND_EXIT_BUFFERED = 10;


int is_log_producer_result_ok(log_producer_result rst)
{
    return rst == LOG_PRODUCER_OK;
}

