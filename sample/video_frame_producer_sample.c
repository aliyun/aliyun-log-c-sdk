//
// Created by ZhangCheng on 26/02/2018.
//

#include "log_api.h"
#include "log_producer_config.h"
#include "log_producer_client.h"
#include "log_multi_thread.h"


/**
 * 发送数据结果的回调函数
 * @param config_name
 * @param result
 * @param log_bytes
 * @param compressed_bytes
 * @param req_id
 * @param message
 */
void on_log_send_done(const char * config_name, log_producer_result result, size_t log_bytes, size_t compressed_bytes, const char * req_id, const char * message, const unsigned char * raw_buffer)
{
    if (result == LOG_PRODUCER_OK)
    {
        printf("send success, config : %s, result : %d, log bytes : %d, compressed bytes : %d, request id : %s \n",
               config_name, (result),
               (int)log_bytes, (int)compressed_bytes, req_id);

    }
    else
    {
        printf("send fail, config : %s, result : %d, log bytes : %d, compressed bytes : %d, request id : %s, error message : %s\n",
               config_name, (result),
               (int)log_bytes, (int)compressed_bytes, req_id, message);
    }

}

/**
 * create producer 的一层封装
 * @param on_send_done
 * @return
 */
log_producer * create_log_producer_wrapper(on_log_producer_send_done_function on_send_done)
{
    log_producer_config * config = create_log_producer_config();
    // endpoint list:  https://help.aliyun.com/document_detail/29008.html
    log_producer_config_set_endpoint(config, "${your_endpoint}");
    log_producer_config_set_project(config, "${your_project}");
    log_producer_config_set_logstore(config, "${your_logstore}");
    log_producer_config_set_access_id(config, "${your_access_key_id}");
    log_producer_config_set_access_key(config, "${your_access_key_secret}");

    // if you do not need topic or tag, comment it
    // 设置主题
    log_producer_config_set_topic(config, "test_topic");

    // 设置tag信息，此tag会附加在每个关键帧上，建议固定的meta信息使用tag设置
    log_producer_config_add_tag(config, "os", "linux");
    log_producer_config_add_tag(config, "version", "0.1.0");
    log_producer_config_add_tag(config, "type", "type_xxx");

    // set resource param
    log_producer_config_set_packet_log_bytes(config, 4*1024*1024);
    log_producer_config_set_packet_log_count(config, 4096);
    // 设置后台数据聚合上传的时间，默认为3000ms，如有需求可自定义设置
    log_producer_config_set_packet_timeout(config, 3000);
    // 最大缓存的数据大小，超出缓存时add_log接口会立即返回失败
    log_producer_config_set_max_buffer_limit(config, 64*1024*1024);
    // 设置不使用压缩
    log_producer_config_set_compress_type(config, 0);

    // set send thread count
    // 发送线程数，为1即可
    log_producer_config_set_send_thread_count(config, 1);

    // on_send_done 为发送的回调函数（发送成功/失败都会返回）
    return create_log_producer(config, on_send_done);
}

#define FRAME_FLAG_KEY_FRAME 1
#define FRAME_FLAG_NONE 0

/**
 * 帧数据的一种封装形式，您可以根据需求自定义调整
 */
typedef struct _video_frame {
    uint64_t duration;
    uint8_t * frame_data;
    uint32_t frame_data_size;
    uint32_t frame_flag;
    uint64_t decoding_timestamp;
    uint64_t presentation_timestamp;
    uint8_t * extra_info;
    uint8_t extra_info_size;
}video_frame;


/**
 * 发送帧数据，这里是一种示例的封装形式，您可以根据需求进行更改
 * @param client producer Client
 * @param frame 帧数据
 */
void post_frame(log_producer_client * client, video_frame * frame)
{
    if (client == NULL || frame == NULL)
    {
        return;
    }

    // 帧数据封装 start
    char * g_frame_keys[] = {
            (char *)"duration",
            (char *)"frame_data",
            (char *)"frame_flag",
            (char *)"decoding_timestamp",
            (char *)"presentation_timestamp",
            (char *)"extra_info"
    };

    size_t g_frame_keys_len[] = {
            strlen("duration"),
            strlen("frame_data"),
            strlen("frame_flag"),
            strlen("decoding_timestamp"),
            strlen("presentation_timestamp"),
            strlen("extra_info")
    };

    char * values[6];

    char duration[32];
    sprintf(duration, "%llu", (long long unsigned int)frame->duration);

    char frame_flag[32];
    sprintf(frame_flag, "%u", frame->frame_flag);

    char decoding_timestamp[32];
    sprintf(decoding_timestamp, "%llu", (long long unsigned int)frame->decoding_timestamp);

    char presentation_timestamp[32];
    sprintf(presentation_timestamp, "%llu", (long long unsigned int)frame->presentation_timestamp);

    values[0] = duration;
    values[1] = (char *)frame->frame_data;
    values[2] = frame_flag;
    values[3] = decoding_timestamp;
    values[4] = presentation_timestamp;
    values[5] = (char *)frame->extra_info;

    size_t values_len[] = {
            strlen(duration),
            frame->frame_data_size,
            strlen(frame_flag),
            strlen(decoding_timestamp),
            strlen(presentation_timestamp),
            frame->extra_info_size
    };

    // 帧数据封装 end


    // 调用producer接口发送数据
    log_producer_result rst = log_producer_client_add_log_with_len(client, 6, g_frame_keys, g_frame_keys_len, values, values_len);
    // 异常处理
    if (rst != LOG_PRODUCER_OK)
    {
        // 这边可以加入一些异常处理，如果是关键帧发送失败，可以每隔10ms重试一次，超过10次继续失败时再丢弃
        if ((frame->frame_flag & FRAME_FLAG_KEY_FRAME) != 0)
        {
            int try_time = 0;
            for (; try_time < 10; ++try_time)
            {
                usleep(10000);
                if (log_producer_client_add_log_with_len(client, 6, g_frame_keys, g_frame_keys_len, values, values_len) == LOG_PRODUCER_OK)
                {
                    break;
                }
            }
            if (try_time == 10)
            {
                printf("add key frame error %d \n", rst);
            }
        }
        else
        {
            printf("add normal frame error %d \n", rst);
        }
    }

}

/**
 * 示例函数
 */
void producer_post_frames()
{
    if (log_producer_env_init(LOG_GLOBAL_ALL) != LOG_PRODUCER_OK) {
        exit(1);
    }

    log_producer * producer = create_log_producer_wrapper(on_log_send_done);
    if (producer == NULL)
    {
        printf("create producer by config fail \n");
        exit(1);
    }

    log_producer_client * client = get_log_producer_client(producer, NULL);
    if (client == NULL)
    {
        printf("create producer client by config fail \n");
        exit(1);
    }

    srand(time(NULL));

    int i = 0;
    for (; i < 100; ++i)
    {
        video_frame frame;
        // 每10帧有一个为关键帧
        frame.frame_flag = i % 10 == 0 ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;

        // mock 帧数据
        uint32_t data_size = 960*720;
        uint8_t * frame_data = (uint8_t *)malloc(data_size);
        memset(frame_data, i % 256, data_size);
        frame.frame_data_size = data_size;
        frame.frame_data = frame_data;

        frame.duration = rand() % 500;
        frame.decoding_timestamp = time(NULL);
        frame.presentation_timestamp = time(NULL);
        char extra_info[32];
        sprintf(extra_info, "extra_%d", i);
        frame.extra_info = (uint8_t *)extra_info;
        frame.extra_info_size = strlen(extra_info);

        // 发送帧数据
        post_frame(client, &frame);

        free(frame_data);

        usleep(30000);
    }

    destroy_log_producer(producer);
    log_producer_env_destroy();
}

int main(int argc, char *argv[])
{
    producer_post_frames();
    return 0;
}

