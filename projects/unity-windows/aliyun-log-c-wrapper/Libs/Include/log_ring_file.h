//
// Created by davidzhang on 2020/8/23.
//

#ifndef LOG_C_SDK_LOG_RING_FILE_H
#define LOG_C_SDK_LOG_RING_FILE_H

#include "log_inner_include.h"

typedef struct _log_ring_file
{
    char * filePath;
    int maxFileCount;
    int maxFileSize;
    int syncWrite;

    int nowFileIndex;
    uint64_t nowOffset;
    int nowFD;
    int *fileRemoveFlags;
    int *fileUseFlags;
}log_ring_file;

log_ring_file * log_ring_file_open(const char * filePath, int maxFileCount, int maxFileSize, int syncWrite);
int log_ring_file_write(log_ring_file * file, uint64_t offset, int buffer_count, const void * buffer[], size_t buffer_size[]);
int log_ring_file_write_single(log_ring_file * file, uint64_t offset, const void * buffer, size_t buffer_size);
int log_ring_file_read(log_ring_file * file, uint64_t offset, void * buffer, size_t buffer_size);
int log_ring_file_flush(log_ring_file * file);
int log_ring_file_clean(log_ring_file * file, uint64_t startOffset, uint64_t endOffset);
int log_ring_file_close(log_ring_file * file);

#endif //LOG_C_SDK_LOG_RING_FILE_H
