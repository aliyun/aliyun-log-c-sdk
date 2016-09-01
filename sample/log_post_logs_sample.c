#include "aos_log.h"
#include "aos_util.h"
#include "aos_string.h"
#include "aos_status.h"
#include "log_auth.h"
#include "log_util.h"
#include "log_api.h"
#include "log_config.h"
#include <time.h>
#include <stdlib.h>
void log_post_logs_sample()
{
    aos_pool_t *p = NULL;
    aos_status_t *s = NULL;
    aos_pool_create(&p, NULL);
    
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "__source__", "127.0.0.1");
    cJSON_AddStringToObject(root, "__topic__", "topic");
    //cJSON *tags = cJSON_CreateObject();
    //cJSON_AddItemToObject(root, "__tags__", tags);
    //cJSON_AddStringToObject(tags, "tag1", "tag1");
    //cJSON_AddStringToObject(tags, "tag2", "tag2");
    cJSON *logs = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "__logs__", logs);
    cJSON *log1 = cJSON_CreateObject();
    cJSON_AddStringToObject(log1, "level", "error1");
    cJSON_AddStringToObject(log1, "message", "c sdk test message1");
    cJSON_AddNumberToObject(log1, "__time__", apr_time_now()/1000000);
    cJSON *log2 = cJSON_CreateObject();
    cJSON_AddStringToObject(log2, "level", "error2");
    cJSON_AddStringToObject(log2, "message", "c sdk test message2");
    cJSON_AddNumberToObject(log2, "__time__", apr_time_now()/1000000);
    cJSON_AddItemToArray(logs, log1);
    cJSON_AddItemToArray(logs, log2);
    s = log_post_logs(p, LOG_ENDPOINT, ACCESS_KEY_ID, ACCESS_KEY_SECRET, PROJECT_NAME, LOGSTORE_NAME, root);
    if (aos_status_is_ok(s)) {
        printf("post logs succeeded\n");
    } else {
        printf("%d\n",s->code);
        printf("%s\n",s->error_code);
        printf("%s\n",s->error_msg);
        printf("%s\n",s->req_id);
        printf("put logs failed\n");
    }
    cJSON_Delete(root);
    aos_pool_destroy(p);
}

double log_build_http_cont_test(const char* path){
    
    apr_initialize();
    
    FILE* f = fopen(path, "r");
    if(!f)  return -1;
    
    const int content_number = 10*2000*28*2;
    char **buf = malloc(sizeof(char*)*content_number);
    int i;
    for(i=0;i<content_number;i++){
        buf[i] = (char*)malloc(sizeof(char)*32);
    }
    char buffer[1024];
    int index = 0;
    while(index<content_number)
    {
        fscanf(f,"%s",buffer);
        if(buffer[0]=='['){
            fscanf(f,"%s",buffer);
            fscanf(f,"%s",buffer);
        }
        else{
            sscanf(buffer, "%[^:]:%[^:]",buf[index],buf[index+1]);
            index+=2;
        }
    }
    
    clock_t start = clock();
    
    int test_time;
    for(test_time = 0;test_time < 100;test_time++)
    {
        
        int log_buffer_index = 0;
        int log_group_index;
        for(log_group_index = 0;log_group_index < 10;log_group_index++)
        {
            log_group_builder* bder = log_group_create();
            int log_index;
            for(log_index = 0;log_index < 2000;log_index++)
            {
                add_log(bder);
                int content_index;
                for(content_index = 0;content_index < 28;content_index++)
                {
                    add_log_key_value(bder, buf[log_buffer_index], strlen(buf[log_buffer_index]), buf[log_buffer_index+1], strlen(buf[log_buffer_index+1]));
                    log_buffer_index+=2;
                }
            }
            log_http_cont* cont =  log_create_http_cont(LOG_ENDPOINT, ACCESS_KEY_ID, ACCESS_KEY_SECRET,NULL, PROJECT_NAME, LOGSTORE_NAME, bder);
            log_clean_http_cont(cont);
        }
        
    }
    clock_t end = clock();
    
    for(i=0;i<content_number;i++){
        free(buf[i]);
    }
    free(buf);
    fclose(f);
    
    apr_terminate();
    return (double)(end - start)/CLOCKS_PER_SEC/100;
    
}

void log_post_logs_sample2(){
    
    if (aos_http_io_initialize("linux-x86_64", 0) != AOSE_OK) {
        exit(1);
    }
    
    aos_status_t *s = NULL;
    log_group_builder* bder = log_group_create();
    add_source(bder,"mSource",sizeof("mSource"));
    add_topic(bder,"mTopic", sizeof("mTopic"));
    
    int i;
    for(i=0;i<3;i++){
        add_log(bder);
        int j;
        for(j=0;j<5;j++){
            char k[] = "_k_";
            char v[] = "_v_";
            add_log_key_value(bder, k, strlen(k), v, strlen(v));
        }
    }
    
    //s = log_post_logs_from_proto_buf(LOG_ENDPOINT, ACCESS_KEY_ID, ACCESS_KEY_SECRET,NULL, PROJECT_NAME, LOGSTORE_NAME, bder);
    
    log_http_cont* cont =  log_create_http_cont(LOG_ENDPOINT, ACCESS_KEY_ID, ACCESS_KEY_SECRET,NULL, PROJECT_NAME, LOGSTORE_NAME, bder);
    s =  log_post_logs_from_http_cont(cont);
    if (aos_status_is_ok(s)) {
        printf("post logs succeeded\n");
    } else {
        printf("%d\n",s->code);
        printf("%s\n",s->error_code);
        printf("%s\n",s->error_msg);
        printf("%s\n",s->req_id);
        printf("put logs failed\n");
    }
    
    log_clean_http_cont(cont);
    
    aos_http_io_deinitialize();
}

int main(int argc, char *argv[])
{
    log_post_logs_sample2();
    return 0;
}
