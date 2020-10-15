//
// Created by davidzhang on 2020/8/23.
//

#include "log_persistent_manager.h"
#include "log_producer_manager.h"
#include "inner_log.h"
#include "log_builder.h"
#include "sds.h"

#define MAX_CHECKPOINT_FILE_SIZE (sizeof(log_persistent_checkpoint) * 1024)
#define LOG_PERSISTENT_HEADER_MAGIC (0xf7216a5b76df67f5)

static int32_t is_valid_log_checkpoint(log_persistent_checkpoint * checkpoint)
{
    return checkpoint->check_sum == checkpoint->start_log_uuid + checkpoint->now_log_uuid +
                                       checkpoint->start_file_offset + checkpoint->now_file_offset;
}

static int32_t recover_log_checkpoint(log_persistent_manager * manager)
{
    FILE * tmpFile = fopen(manager->checkpoint_file_path, "rb");
    if (tmpFile == NULL)
    {
        if (errno == ENOENT)
        {
            return 0;
        }
        return -1;
    }
    fseek(tmpFile, 0, SEEK_END);
    long pos = ftell(tmpFile);
    if (pos == 0)
    {
        // empty file
        return 0;
    }
    long fixedPos = pos - pos%sizeof(log_persistent_checkpoint);
    long lastPos = fixedPos == 0? 0 : fixedPos - sizeof(log_persistent_checkpoint);
    fseek(tmpFile, lastPos, SEEK_SET);
    if (1 != fread((void *)&(manager->checkpoint), sizeof(log_persistent_checkpoint), 1, tmpFile))
    {
        fclose(tmpFile);
        return -2;
    }
    if (!is_valid_log_checkpoint(&(manager->checkpoint)))
    {
        fclose(tmpFile);
        return -3;
    }
    fclose(tmpFile);
    manager->checkpoint_file_size = pos;
    return 0;
}

int save_log_checkpoint(log_persistent_manager * manager)
{
    log_persistent_checkpoint * checkpoint = &(manager->checkpoint);
    checkpoint->check_sum = checkpoint->start_log_uuid + checkpoint->now_log_uuid +
            checkpoint->start_file_offset + checkpoint->now_file_offset;
    if (manager->checkpoint_file_size >= MAX_CHECKPOINT_FILE_SIZE)
    {
        if (manager->checkpoint_file_ptr != NULL)
        {
            fclose(manager->checkpoint_file_ptr);
            manager->checkpoint_file_ptr = NULL;
        }
        char tmpFilePath[256];
        strcpy(tmpFilePath, manager->checkpoint_file_path);
        strcat(tmpFilePath, ".bak");
        aos_info_log("start switch checkpoint index file %s \n", manager->checkpoint_file_path);
        FILE * tmpFile = fopen(tmpFilePath, "wb+");
        if (tmpFile == NULL)
            return -1;
        if (1 !=
            fwrite((const void *)(&manager->checkpoint), sizeof(log_persistent_checkpoint), 1, tmpFile))
        {
            fclose(tmpFile);
            return -2;
        }
        if (fclose(tmpFile) != 0)
            return -3;
        if (rename(tmpFilePath, manager->checkpoint_file_path) != 0 )
            return -4;
        manager->checkpoint_file_size = sizeof(log_persistent_checkpoint);
        return 0;
    }
    if (manager->checkpoint_file_ptr == NULL)
    {
        manager->checkpoint_file_ptr = fopen(manager->checkpoint_file_path, "ab+");
        if (manager->checkpoint_file_ptr == NULL)
            return -5;
    }
    if (1 !=
        fwrite((const void *)(&manager->checkpoint), sizeof(log_persistent_checkpoint), 1, manager->checkpoint_file_ptr))
        return -6;
    if (fflush(manager->checkpoint_file_ptr) != 0)
        return -7;
    manager->checkpoint_file_size += sizeof(log_persistent_checkpoint);
    return 0;
}

void on_log_persistent_manager_send_done_uuid(const char * config_name,
                                              log_producer_result result,
                                              size_t log_bytes,
                                              size_t compressed_bytes,
                                              const char * req_id,
                                              const char * error_message,
                                              const unsigned char * raw_buffer,
                                              void *persistent_manager,
                                              int64_t startId,
                                              int64_t endId)
{
    if (result != LOG_PRODUCER_OK && result != LOG_PRODUCER_DROP_ERROR && result != LOG_PRODUCER_INVALID)
    {
        return;
    }
    log_persistent_manager * manager = (log_persistent_manager *)persistent_manager;
    if (manager == NULL)
    {
        return;
    }
    if (manager->is_invalid)
    {
        return;
    }
    if (startId < 0 || endId < 0 || startId > endId || endId - startId > 1024 * 1024)
    {
        aos_fatal_log("invalid id range %lld %lld", startId, endId);
        manager->is_invalid = 1;
        return;
    }

    // multi thread send is not allowed, and this should never happen
    if (startId > manager->checkpoint.start_log_uuid)
    {
        aos_fatal_log("project %s, logstore %s, invalid checkpoint start log uuid %lld %lld",
                      manager->config->project,
                      manager->config->logstore,
                      startId,
                      manager->checkpoint.start_log_uuid);
        manager->is_invalid = 1;
        return;
    }
    CS_ENTER(manager->lock);
    // cal log size
    uint64_t totalOffset = 0;
    for (int64_t id = startId; id <= endId; ++id)
    {
        totalOffset += manager->in_buffer_log_sizes[id % manager->config->maxPersistentLogCount];
    }

    manager->checkpoint.start_file_offset += totalOffset;
    manager->checkpoint.start_log_uuid = endId + 1;
    int rst = save_log_checkpoint(manager);
    if (rst != 0)
    {
        aos_error_log("project %s, logstore %s, save checkpoint failed, reason %d",
                      manager->config->project,
                      manager->config->logstore,
                      rst);
    }
    log_ring_file_clean(manager->ring_file, manager->checkpoint.start_file_offset - totalOffset, manager->checkpoint.start_file_offset);

    CS_LEAVE(manager->lock);
}


static void log_persistent_manager_init(log_persistent_manager * manager, log_producer_config *config)
{
    memset(manager, 0, sizeof(log_persistent_manager));
    manager->builder = log_group_create();
    manager->checkpoint.start_log_uuid = (int64_t)(time(NULL)) * 1000LL * 1000LL * 1000LL;
    manager->checkpoint.now_log_uuid = manager->checkpoint.start_log_uuid;
    manager->config = config;
    manager->lock = CreateCriticalSection();
    manager->in_buffer_log_sizes = (uint32_t *)malloc(sizeof(uint32_t) * config->maxPersistentLogCount);
    manager->checkpoint_file_path = sdscat(sdsdup(config->persistentFilePath), ".idx");
    memset(manager->in_buffer_log_sizes, 0, sizeof(uint32_t) * config->maxPersistentLogCount);
    manager->ring_file = log_ring_file_open(config->persistentFilePath, config->maxPersistentFileCount, config->maxPersistentFileSize, config->forceFlushDisk);
}

static void log_persistent_manager_clear(log_persistent_manager * manager)
{
    log_group_destroy(manager->builder);
    ReleaseCriticalSection(manager->lock);
    if (manager->checkpoint_file_ptr != NULL)
    {
        fclose(manager->checkpoint_file_ptr);
        manager->checkpoint_file_ptr = NULL;
    }
    free(manager->in_buffer_log_sizes);
    sdsfree(manager->checkpoint_file_path);
    log_ring_file_close(manager->ring_file);
}

log_persistent_manager *
create_log_persistent_manager(log_producer_config *config)
{
    if (!log_producer_persistent_config_is_enabled(config))
    {
        return NULL;
    }
    log_persistent_manager * manager = (log_persistent_manager *)malloc(sizeof(log_persistent_manager));
    log_persistent_manager_init(manager, config);
    return manager;
}

void destroy_log_persistent_manager(log_persistent_manager *manager)
{
    if (manager == NULL)
    {
        return;
    }
    log_persistent_manager_clear(manager);
    free(manager);
}

int log_persistent_manager_save_log(log_persistent_manager *manager,
                                    const char *logBuf, size_t logSize)
{
    // save binlog
    const void * buffer[2];
    size_t bufferSize[2];

    log_persistent_item_header header;
    header.magic_code = LOG_PERSISTENT_HEADER_MAGIC;
    header.log_uuid = manager->checkpoint.now_log_uuid;
    header.log_size = logSize;
    header.preserved = 0;

    buffer[0] = &header;
    buffer[1] = logBuf;
    bufferSize[0] = sizeof(log_persistent_item_header);
    bufferSize[1] = logSize;
    int rst = log_ring_file_write(manager->ring_file, manager->checkpoint.now_file_offset, 2, buffer, bufferSize);
    if (rst != bufferSize[0] + bufferSize[1])
    {
        aos_error_log("project %s, logstoe %s, write bin log failed, rst %d",
                      manager->config->project,
                      manager->config->logstore,
                      rst);
        return LOG_PRODUCER_PERSISTENT_ERROR;
    }
    // update in memory checkpoint
    manager->in_buffer_log_sizes[manager->checkpoint.now_log_uuid % manager->config->maxPersistentLogCount] = rst;
    manager->checkpoint.now_file_offset += rst;
    ++manager->checkpoint.now_log_uuid;
    aos_debug_log("project %s, logstore %s, write bin log success, offset %lld, uuid %lld, log size %d",
                  manager->config->project,
                  manager->config->logstore,
                  manager->checkpoint.now_file_offset,
                  manager->checkpoint.now_log_uuid,
                  rst);
    if (manager->first_checkpoint_saved == 0)
    {
        save_log_checkpoint(manager);
        manager->first_checkpoint_saved = 1;
    }
    return 0;
}

int log_persistent_manager_is_buffer_enough(log_persistent_manager *manager,
                                            size_t logSize)
{
    if (manager->checkpoint.now_file_offset - manager->checkpoint.start_file_offset + logSize + 1024 >
        (uint64_t)manager->config->maxPersistentFileCount * manager->config->maxPersistentFileSize &&
        manager->checkpoint.now_log_uuid - manager->checkpoint.start_log_uuid < manager->config->maxPersistentLogCount - 1)
    {
        return 0;
    }
    return 1;
}

static int log_persistent_manager_recover_inner(log_persistent_manager *manager,
                                                log_producer_manager *producer_manager)
{
    int rst = recover_log_checkpoint(manager);
    if (rst != 0)
    {
        return rst;
    }

    aos_info_log("project %s, logstore %s, recover persistent checkpoint success, checkpoint %lld %lld %lld %lld",
                 manager->config->project,
                 manager->config->logstore,
                 manager->checkpoint.start_file_offset,
                 manager->checkpoint.now_file_offset,
                 manager->checkpoint.start_log_uuid,
                 manager->checkpoint.now_log_uuid);

    if (manager->checkpoint.start_file_offset == 0 && manager->checkpoint.now_file_offset == 0)
    {
        // no need to recover
        return 0;
    }

    // try recover ring file

    log_persistent_item_header header;

    uint64_t fileOffset = manager->checkpoint.start_file_offset;
    int64_t logUUID = manager->checkpoint.start_log_uuid;

    char * buffer = NULL;
    int max_buffer_size = 0;

    while (1)
    {
        rst = log_ring_file_read(manager->ring_file, fileOffset, &header, sizeof(log_persistent_item_header));
        if (rst != sizeof(log_persistent_item_header))
        {
            if (rst == 0)
            {
                aos_info_log("project %s, logstore %s, read end of file",
                             manager->config->project,
                             manager->config->logstore);
                if (buffer != NULL)
                {
                    free(buffer);
                    buffer = NULL;
                }
                break;
            }
            aos_error_log("project %s, logstore %s, read binlog file header failed, offset %lld, result %d",
                          manager->config->project,
                          manager->config->logstore,
                          fileOffset,
                          rst);
            if (buffer != NULL)
            {
                free(buffer);
                buffer = NULL;
            }
            return -1;
        }
        if (header.magic_code != LOG_PERSISTENT_HEADER_MAGIC ||
            header.log_uuid < logUUID ||
            header.log_size <= 0 || header.log_size > 10*1024*1024 )
        {
            aos_info_log("project %s, logstore %s, read binlog file success, uuid %lld %lld",
                          manager->config->project,
                          manager->config->logstore,
                          header.log_uuid,
                          logUUID);
            break;
        }
        if (buffer == NULL || max_buffer_size < header.log_size)
        {
            if (buffer != NULL)
            {
                free(buffer);
            }
            buffer = (char *)malloc(header.log_size * 2);
            max_buffer_size = header.log_size * 2;
        }
        rst = log_ring_file_read(manager->ring_file, fileOffset + sizeof(log_persistent_item_header), buffer, header.log_size);
        if (rst != header.log_size)
        {
            // if read fail, just break
            aos_warn_log("project %s, logstore %s, read binlog file content failed, offset %lld, result %d",
                          manager->config->project,
                          manager->config->logstore,
                          fileOffset + sizeof(log_persistent_item_header),
                          rst);
            break;
        }
        if (header.log_uuid - logUUID > 1024*1024)
        {
            aos_error_log("project %s, logstore %s, log uuid jump, %lld %lld",
                          manager->config->project,
                          manager->config->logstore,
                          header.log_uuid,
                          logUUID);
            if (buffer != NULL)
            {
                free(buffer);
                buffer = NULL;
            }
            return -3;
        }
        // set empty log uuid len 0
        for (int64_t emptyUUID = logUUID + 1; emptyUUID < header.log_uuid; ++emptyUUID)
        {
            manager->in_buffer_log_sizes[emptyUUID % manager->config->maxPersistentLogCount] = 0;
        }
        manager->in_buffer_log_sizes[header.log_uuid % manager->config->maxPersistentLogCount] = header.log_size + sizeof(log_persistent_item_header);

        logUUID = header.log_uuid;
        fileOffset += header.log_size + sizeof(log_persistent_item_header);

        rst = log_producer_manager_add_log_raw(producer_manager, buffer, header.log_size, 0, header.log_uuid);
        if (rst != 0)
        {
            aos_error_log("project %s, logstore %s, add log to producer manager failed, this log will been dropped",
                          manager->config->project,
                          manager->config->logstore);
        }

    }
    if (buffer != NULL)
    {
        free(buffer);
        buffer = NULL;
    }

    if (logUUID < manager->checkpoint.now_log_uuid - 1)
    {
        // replay fail
        aos_fatal_log("project %s, logstore %s, replay bin log failed, now log uuid %lld, expected min log uuid %lld, start uuid %lld, start offset  %lld, now offset  %lld, replayed offset %lld",
                      manager->config->project,
                      manager->config->logstore,
                      logUUID,
                      manager->checkpoint.now_log_uuid,
                      manager->checkpoint.start_log_uuid,
                      manager->checkpoint.start_file_offset,
                      manager->checkpoint.now_file_offset,
                      fileOffset);
        return -4;
    }

    // update new checkpoint when replay bin log success
    if (fileOffset > manager->checkpoint.start_file_offset)
    {
        manager->checkpoint.now_log_uuid = logUUID + 1;
        manager->checkpoint.now_file_offset = fileOffset;
    }

    aos_info_log("project %s, logstore %s, replay bin log success, now checkpoint %lld %lld %lld %lld",
                 manager->config->project,
                 manager->config->logstore,
                 manager->checkpoint.start_log_uuid,
                 manager->checkpoint.now_log_uuid,
                 manager->checkpoint.start_file_offset,
                 manager->checkpoint.now_file_offset);

    // save new checkpoint
    rst = save_log_checkpoint(manager);
    if (rst != 0)
    {
        aos_error_log("project %s, logstore %s, save checkpoint failed, reason %d",
                      manager->config->project,
                      manager->config->logstore,
                      rst);
    }
    return rst;
}

static void log_persistent_manager_reset(log_persistent_manager * manager)
{
    log_producer_config * config = manager->config;
    log_persistent_manager_clear(manager);
    log_persistent_manager_init(manager, config);
    manager->checkpoint.start_log_uuid = (int64_t)(time(NULL)) * 1000LL * 1000LL * 1000LL + 500LL * 1000LL * 1000LL;
    manager->checkpoint.now_log_uuid = manager->checkpoint.start_log_uuid;
    manager->is_invalid = 0;
}

int log_persistent_manager_recover(log_persistent_manager *manager,
                                   log_producer_manager *producer_manager)
{
    aos_info_log("project %s, logstore %s, start recover persistent manager",
                 manager->config->project,
                 manager->config->logstore);
    CS_ENTER(manager->lock);
    int rst = log_persistent_manager_recover_inner(manager, producer_manager);
    if (rst != 0)
    {
        // if recover failed, reset persistent manager
        manager->is_invalid = 1;
        log_persistent_manager_reset(manager);
    }
    else
    {
        manager->is_invalid = 0;
    }
    CS_LEAVE(manager->lock);
    return rst;
}
