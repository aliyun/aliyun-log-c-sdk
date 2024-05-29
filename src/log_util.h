#ifndef LIBLOG_UTIL_H
#define LIBLOG_UTIL_H

#include "log_define.h"
#include <stdio.h>
LOG_CPP_START

void md5_to_string(const char * buffer, int bufLen, char * md5);

int signature_to_base64(const char * sig, int sigLen, const char * key, int keyLen, char * base64);

/** open file, see function [fopen]
 *
 * @param path: file path, in utf-8
 */
FILE *log_sys_fopen(const char *path, const char *mode);

/** open file, see function [open]
 *
 * @param path: file path, in utf-8
 */
int log_sys_open(const char *file, int flag, int mode);
/** open file in sync mode, O_SYNC, see function [open]
 *
 * @param path: file path, in utf-8
 */
int log_sys_open_sync(const char *file, int flag, int mode);

/** flush file, see function [fsync]
 *
 */
int log_sys_fsync(int fd);

/** sleep
 *
 */
void log_sleep_ms(int ms);
LOG_CPP_END
#endif
