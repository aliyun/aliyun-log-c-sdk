//
// Created by ZhangCheng on 29/12/2017.
//
#include "log_queue.h"
#include "log_multi_thread.h"


struct _log_queue{
    void ** data;
    int64_t head;
    int64_t tail;
    int32_t size;
    CRITICALSECTION mutex;
    COND notempty;
};

log_queue * log_queue_create(int32_t size)
{
    void * buffer = malloc(sizeof(void *) * size + sizeof(log_queue));
    memset(buffer, 0, sizeof(void *) * size + sizeof(log_queue));
    log_queue * queue = (log_queue *)buffer;
    queue->data = (void **)(buffer + sizeof(log_queue));
    queue->size = size;
    queue->mutex = CreateCriticalSection();
    queue->notempty = CreateCond();
    return queue;
}

void log_queue_destroy(log_queue * queue)
{
    DeleteCriticalSection(queue->mutex);
    DeleteCond(queue->notempty);
    free(queue);
}

int32_t log_queue_size(log_queue * queue)
{
    CS_ENTER(queue->mutex);
    int32_t len = queue->tail - queue->head;
    CS_LEAVE(queue->mutex);
    return len;
}

int32_t log_queue_isfull(log_queue * queue)
{
    CS_ENTER(queue->mutex);
    int32_t rst = (int32_t)((queue->tail - queue->head) == queue->size);
    CS_LEAVE(queue->mutex);
    return rst;
}

int32_t log_queue_push(log_queue * queue, void * data)
{
    CS_ENTER(queue->mutex);
    if (queue->tail - queue->head == queue->size)
    {
        CS_LEAVE(queue->mutex);
        return -1;
    }
    queue->data[queue->tail++ % queue->size] = data;
    CS_LEAVE(queue->mutex);
    COND_SIGNAL(queue->notempty);
    return 0;
}

void * log_queue_pop(log_queue * queue, int32_t waitMs) {
    CS_ENTER(queue->mutex);
    if (queue->tail == queue->head) {
        COND_WAIT_TIME(queue->notempty, queue->mutex, waitMs);
    }
    void * result = NULL;
    if (queue->tail > queue->head)
    {
        result = queue->data[queue->head++ % queue->size];
    }
    CS_LEAVE(queue->mutex);
    return result;
}

void * log_queue_trypop(log_queue * queue)
{
    CS_ENTER(queue->mutex);
    void * result = NULL;
    if (queue->tail > queue->head)
    {
        result = queue->data[queue->head++ % queue->size];
    }
    CS_LEAVE(queue->mutex);
    return result;
}

