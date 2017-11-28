//
//  log_http_cont.h
//  log-c-sdk-new
//
//  Created by 王佳玮 on 16/8/31.
//  Copyright © 2016年 wangjwchn. All rights reserved.
//

#ifndef log_http_cont_h
#define log_http_cont_h

#include "aos_util.h"
#include "aos_log.h"
#include "log_define.h"
#include "log_builder.h"

log_http_cont* log_create_http_cont(const char *endpoint, const char * accesskeyId, const char *accessKey, const char *stsToken,const char* project,const char* logstore,log_group_builder* bder);
log_http_cont* log_create_http_cont_with_lz4_data(const char *endpoint, const char * accesskeyId, const char *accessKey, const char *stsToken,const char* project,const char* logstore, lz4_log_buf * pLZ4buf, apr_pool_t *root);

void log_clean_http_cont(log_http_cont* cont);

#endif /* log_http_cont_h */
