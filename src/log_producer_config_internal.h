//
// Internal header for log_producer_config
// This file should ONLY be included by SDK internal source files
// DO NOT expose this to users
//

#ifndef LOG_C_SDK_LOG_PRODUCER_CONFIG_INTERNAL_H
#define LOG_C_SDK_LOG_PRODUCER_CONFIG_INTERNAL_H

#include "log_producer_config.h"
#include "sds.h"

/**
 * Internal definition of credentials structure
 * @note This is ONLY for internal SDK use
 * @note Users should NEVER see this definition
 * @note We use sds internally for better string management
 */
struct _log_producer_credentials
{
    sds access_key_id;
    sds access_key_secret;
    sds security_token;
    int64_t expire_ts;
};

/**
 * Internal function: create credentials instance
 * @note SDK internal use only
 * @return empty credentials
 */
log_producer_credentials * log_producer_credentials_create();

/**
 * Internal function: destroy credentials instance
 * @note SDK internal use only
 * @param credentials
 */
void log_producer_credentials_destroy(log_producer_credentials * credentials);

#endif //LOG_C_SDK_LOG_PRODUCER_CONFIG_INTERNAL_H

