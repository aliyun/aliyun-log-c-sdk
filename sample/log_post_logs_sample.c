#include "log_util.h"
#include "log_api.h"
#include "log_config.h"
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define POST_LOG_COUNT 10
#define POST_LOG_CONTENT_PAIRS 15

void post_logs_with_http_cont_lz4_log_option()
{

    sls_log_init();

    int i = 0;
    for (i = 0; 1; ++i)
    {
        log_group_builder *bder = log_group_create();
        add_source(bder, "10.230.201.117", sizeof("10.230.201.117"));
        add_topic(bder, "post_logs_with_http_cont_lz4_log_option",
                  sizeof("post_logs_with_http_cont_lz4_log_option"));

        add_tag(bder, "taga_key", strlen("taga_key"), "taga_value",
                strlen("taga_value"));
        add_tag(bder, "tagb_key", strlen("tagb_key"), "tagb_value",
                strlen("tagb_value"));
        add_pack_id(bder, "123456789ABC", strlen("123456789ABC"), 0);

        int x;
        for (x = 0; x < POST_LOG_COUNT; x++)
        {
            char * keys[POST_LOG_CONTENT_PAIRS];
            char * values[POST_LOG_CONTENT_PAIRS];
            size_t key_lens[POST_LOG_CONTENT_PAIRS];
            size_t val_lens[POST_LOG_CONTENT_PAIRS];
            int j = 0;
            for (j = 0; j < POST_LOG_CONTENT_PAIRS; j++)
            {

                keys[j] = (char *)malloc(64);
                sprintf(keys[j], "key_%d", x * j);
                values[j] = (char *)malloc(64);
                sprintf(values[j], "value_%d", 2 * j);
                key_lens[j] = strlen(keys[j]);
                val_lens[j] = strlen(values[j]);
            }
            add_log_full(bder, POST_LOG_CONTENT_PAIRS, keys, key_lens, values, val_lens);
            add_log_time(bder, time(NULL));
            for (j = 0; j < POST_LOG_CONTENT_PAIRS; j++)
            {
                free(keys[j]);
                free(values[j]);
            }
        }

        lz4_log_buf *pLZ4Buf = serialize_to_proto_buf_with_malloc_lz4(bder);
        log_group_destroy(bder);
        if (pLZ4Buf == NULL)
        {
            printf("serialize_to_proto_buf_with_malloc_lz4 failed\n");
            exit(1);
        }
        post_log_result * rst = post_logs_from_lz4buf(LOG_ENDPOINT, ACCESS_KEY_ID,
                                                 ACCESS_KEY_SECRET, NULL,
                                                 PROJECT_NAME, LOGSTORE_NAME,
                                                 pLZ4Buf);
        printf("result %d %d \n", i, rst->statusCode);
        if (rst->errorMessage != NULL)
        {
            printf("error message %s \n", rst->errorMessage);
        }

        if (rst->requestID != NULL)
        {
            printf("requestID %s \n", rst->requestID);
        }
        post_log_result_destroy(rst);
        free_lz4_log_buf(pLZ4Buf);
    }

    sls_log_destroy();
}

int main(int argc, char *argv[])
{
    post_logs_with_http_cont_lz4_log_option();
    return 0;
}
