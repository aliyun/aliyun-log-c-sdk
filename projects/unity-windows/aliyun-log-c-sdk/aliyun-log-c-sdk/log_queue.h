//
// Created by ZhangCheng on 29/12/2017.
//

#ifndef LOG_C_SDK_LOG_QUEUE_H
#define LOG_C_SDK_LOG_QUEUE_H

#include <stdint.h>

typedef struct _log_queue log_queue;

log_queue * log_queue_create(int32_t max_size);

void log_queue_destroy(log_queue * queue);

int32_t log_queue_size(log_queue * queue);

int32_t log_queue_isfull(log_queue * queue);


int32_t log_queue_push(log_queue * queue, void * data);

void * log_queue_pop(log_queue * queue, int32_t waitMs);

void * log_queue_trypop(log_queue * queue);

#endif //LOG_C_SDK_LOG_QUEUE_H
