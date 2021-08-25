//
// Created by davidzhang on 2020/8/23.
//

#include "log_ring_file.h"
#include "sds.h"
#include "inner_log.h"
#include <fcntl.h>

static void get_ring_file_offset(log_ring_file * file,
                                 uint64_t offset,
                                 int32_t * fileIndex,
                                 int32_t * fileOffset)
{
    *fileIndex = (offset / file->maxFileSize) % file->maxFileCount;
    *fileOffset = offset % file->maxFileSize;
}

static int log_ring_file_open_fd(log_ring_file *file, uint64_t offset, int32_t fileIndex, int32_t fileOffset)
{
    if (file->nowFD > 0 && file->nowFileIndex == fileIndex && file->nowOffset % file->maxFileSize == fileOffset)
    {
        return 0;
    }
    if (file->nowFD > 0)
    {
        close(file->nowFD);
        file->nowFD = -1;
    }
    file->fileRemoveFlags[fileIndex] = 0;
    char filePath[256] = {0};
    snprintf(filePath, 255, "%s_%03d", file->filePath, fileIndex);
    int openFlag = O_RDWR|O_CREAT;
    if (file->syncWrite)
    {
        openFlag |= O_SYNC;
    }
    file->nowFD = open(filePath, openFlag, 0644);
    if (file->nowFD < 0)
    {
        aos_error_log("open file failed %s, error %s", filePath, strerror(errno));
        return -1;
    }
    if (fileOffset != 0)
    {
        lseek(file->nowFD, fileOffset, SEEK_SET);
    }
    file->nowFileIndex = fileIndex;
    file->nowOffset = offset;
    return 0;
}

log_ring_file *
log_ring_file_open(const char *filePath, int maxFileCount, int maxFileSize, int syncWrite)
{
    log_ring_file * file = (log_ring_file *)malloc(sizeof(log_ring_file));
    memset(file, 0, sizeof(log_ring_file));
    file->filePath = sdsdup((const sds)filePath);
    file->nowFD = -1;
    file->maxFileCount = maxFileCount;
    file->maxFileSize = maxFileSize;
    file->syncWrite = syncWrite;
    file->fileRemoveFlags = (int *)malloc(sizeof(int) * file->maxFileCount);
    memset(file->fileRemoveFlags, 0, sizeof(int) * file->maxFileCount);
    file->fileUseFlags = (int *)malloc(sizeof(int) * file->maxFileCount);
    memset(file->fileUseFlags, 0, sizeof(int) * file->maxFileCount);
    return file;
}


int log_ring_file_write_single(log_ring_file *file, uint64_t offset,
                               const void *buffer,
                               size_t buffer_size)
{

//    {
//        int openFlag = O_RDWR|O_CREAT;
//        if (file->syncWrite)
//        {
//            openFlag |= O_SYNC;
//        }
//        file->nowFD = open(file->filePath, openFlag, 0644);
//        if (file->nowFD < 0)
//        {
//            aos_error_log("open file failed %s, error %s", file->filePath, strerror(errno));
//            return -1;
//        }
//        if (offset != 0)
//        {
//            lseek(file->nowFD, offset, SEEK_SET);
//        }
//        int rst = write(file->nowFD, (char *)buffer, buffer_size);
//        close(file->nowFD);
//        file->nowFD = 0;
//        return rst;
//    }

    int32_t fileIndex = 0;
    int32_t fileOffset = 0;
    size_t nowOffset = 0;
    while (nowOffset < buffer_size)
    {
        get_ring_file_offset(file, offset + nowOffset, &fileIndex, &fileOffset);
        if (log_ring_file_open_fd(file, offset, fileIndex, fileOffset) != 0)
        {
            return -1;
        }

        int writeSize = buffer_size - nowOffset;
        if (file->maxFileSize - fileOffset <= writeSize)
        {
            writeSize = file->maxFileSize - fileOffset;
        }

        int rst = write(file->nowFD, (char *)buffer + nowOffset, writeSize);
        if (rst != writeSize)
        {
            aos_error_log("write buffer to file failed, file %s, offset %d, size %d, error %s",
                          file->filePath,
                          fileIndex + nowOffset,
                          file->maxFileSize - fileOffset,
                          strerror(errno));
            return -1;
        }
        nowOffset += rst;
        file->nowOffset += rst;
    }
    return buffer_size;
}

int log_ring_file_write(log_ring_file *file, uint64_t offset, int buffer_count,
                        const void **buffer, size_t *buffer_size)
{
    uint64_t inner_offset = 0;
    for (int (i) = 0; (i) < buffer_count; ++(i))
    {
        if (log_ring_file_write_single(file, offset + inner_offset, buffer[i], buffer_size[i]) != buffer_size[i])
        {
            return -1;
        }
        inner_offset += buffer_size[i];
    }
    return inner_offset;
}

int log_ring_file_read(log_ring_file *file, uint64_t offset, void *buffer,
                       size_t buffer_size)
{
//    {
//        int openFlag = O_RDWR|O_CREAT;
//        if (file->syncWrite)
//        {
//            openFlag |= O_SYNC;
//        }
//        file->nowFD = open(file->filePath, openFlag, 0644);
//        if (file->nowFD < 0)
//        {
//            aos_error_log("open file failed %s, error %s", file->filePath, strerror(errno));
//            return -1;
//        }
//        if (offset != 0)
//        {
//            lseek(file->nowFD, offset, SEEK_SET);
//        }
//        int rst = read(file->nowFD, buffer, buffer_size);
//        close(file->nowFD);
//        file->nowFD = 0;
//        return rst;
//    }
    int32_t fileIndex = 0;
    int32_t fileOffset = 0;
    size_t nowOffset = 0;
    while (nowOffset < buffer_size)
    {
        get_ring_file_offset(file, offset + nowOffset, &fileIndex, &fileOffset);
        if (log_ring_file_open_fd(file, offset, fileIndex, fileOffset) != 0)
        {
            return -1;
        }
        int rst = 0;
        int readSize = buffer_size - nowOffset;
        if (readSize > file->maxFileSize - fileOffset)
        {
            readSize = file->maxFileSize - fileOffset;
        }
        if ((rst = read(file->nowFD, buffer + nowOffset, readSize)) != readSize)
        {
            if (errno == ENOENT)
            {
                return 0;
            }
            if (rst > 0)
            {
                file->nowOffset += rst;
                nowOffset += rst;
                continue;
            }
            if (rst == 0)
            {
                return file->nowOffset - offset;
            }
            aos_error_log("read buffer from file failed, file %s, offset %d, size %d, error %s",
                          file->filePath,
                          fileIndex + nowOffset,
                          file->maxFileSize - fileOffset,
                          strerror(errno));
            return -1;
        }
        nowOffset += file->maxFileSize - fileOffset;
        file->nowOffset += file->maxFileSize - fileOffset;
    }
    return buffer_size;
}

int log_ring_file_flush(log_ring_file *file)
{
    if (file->nowFD > 0)
    {
        return fsync(file->nowFD);
    }
    return -1;
}

void log_ring_file_remove_file(log_ring_file *file, int32_t fileIndex)
{
    if (file->fileRemoveFlags[fileIndex] > 0)
    {
        return;
    }
    char filePath[256] = {0};
    snprintf(filePath, 255, "%s_%03d", file->filePath, fileIndex);
    remove(filePath);
    aos_info_log("remove file %s", filePath);
    file->fileRemoveFlags[fileIndex] = 1;
}

int log_ring_file_clean(log_ring_file *file, uint64_t startOffset,
                        uint64_t endOffset)
{
    if (endOffset > file->nowOffset)
    {
        aos_error_log("try to clean invalid ring file %s, start %lld, end %lld, now %lld",
                      file->filePath,
                      startOffset,
                      endOffset,
                      file->nowOffset);
        return -1;
    }
    if ((file->nowOffset - endOffset) / file->maxFileSize >= file->maxFileCount - 1)
    {
        // no need to clean
        return 0;
    }
    memset(file->fileUseFlags, 0, sizeof(int) * file->maxFileCount);
    for (int64_t i = endOffset / file->maxFileSize; i <= file->nowOffset / file->maxFileSize; ++i)
    {
        file->fileUseFlags[i % file->maxFileCount] = 1;
    }
    aos_info_log("remove file %s , offset from %lld to %lld, file offset %lld, index from %d to %d",
                 file->filePath,
                 startOffset,
                 endOffset,
                 file->nowOffset,
                 endOffset / file->maxFileSize,
                 file->nowOffset / file->maxFileSize);
    for (int i = 0; i < file->maxFileCount; ++i)
    {
        if (file->fileUseFlags[i] != 0)
            continue;
        log_ring_file_remove_file(file, i);
    }

    return 0;
}

int log_ring_file_close(log_ring_file *file)
{
    if (file != NULL)
    {
        sdsfree(file->filePath);
        if (file->nowFD > 0)
        {
            close(file->nowFD);
            file->nowFD = -1;
        }
        free(file->fileRemoveFlags);
        free(file->fileUseFlags);
        free(file);
    }
    return 0;
}

