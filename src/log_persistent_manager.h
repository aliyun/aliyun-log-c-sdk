//
// Created by davidzhang on 2020/8/23.
//

#include "log_inner_include.h"
#include "log_producer_config.h"
#include "log_ring_file.h"
#include "log_builder.h"
#include "log_producer_manager.h"

#ifndef LOG_C_SDK_LOG_PERSISTENT_MANAGER_H
#define LOG_C_SDK_LOG_PERSISTENT_MANAGER_H

typedef struct _log_persistent_checkpoint {
    uint64_t version;
    unsigned char signature[16]; // persistent config signature
    uint64_t start_file_offset; // logic file start offset in buffer
    uint64_t now_file_offset; // logic now file offset
    int64_t start_log_uuid; // start log uuid in buffer
    int64_t now_log_uuid; // now log uuid
    uint64_t check_sum; // start_file_offset + now_file_offset + start_log_uuid + now_log_uuid
    unsigned char preserved[32];
}log_persistent_checkpoint;

typedef struct _log_persistent_item_header
{
    uint64_t magic_code;
    int64_t log_uuid;
    int64_t log_size;
    uint64_t preserved;
}log_persistent_item_header;

typedef struct _log_persistent_manager{
    CRITICALSECTION lock;
    log_persistent_checkpoint checkpoint;
    uint32_t * in_buffer_log_sizes;
    log_producer_config * config;
    log_group_builder * builder;
    int8_t is_invalid;
    int8_t first_checkpoint_saved;
    log_ring_file * ring_file;

    FILE * checkpoint_file_ptr;
    char * checkpoint_file_path;
    size_t checkpoint_file_size;
}log_persistent_manager;


log_persistent_manager * create_log_persistent_manager(log_producer_config * config);
void destroy_log_persistent_manager(log_persistent_manager * manager);

void on_log_persistent_manager_send_done_uuid(const char * config_name,
                                               log_producer_result result,
                                               size_t log_bytes,
                                               size_t compressed_bytes,
                                               const char * req_id,
                                               const char * error_message,
                                               const unsigned char * raw_buffer,
                                               void *persistent_manager,
                                               int64_t startId,
                                               int64_t endId);

int log_persistent_manager_save_log(log_persistent_manager * manager, const char * logBuf, size_t logSize);
int log_persistent_manager_is_buffer_enough(log_persistent_manager * manager, size_t logSize);

int save_log_checkpoint(log_persistent_manager * manager);

int log_persistent_manager_recover(log_persistent_manager * manager, log_producer_manager * producer_manager);

#endif //LOG_C_SDK_LOG_PERSISTENT_MANAGER_H
