#ifdef _WIN32
#include <windows.h>
#else
#include <arpa/inet.h>
#include <unistd.h>
#endif
#include "hmac-sha.h"
#include "inner_log.h"
#include "log_util.h"
#include "md5.h"
#include <fcntl.h>
#include <string.h>

static const char *g_hex_hash = "0123456789ABCDEF";

void md5_to_string(const char * buffer, int bufLen, char * md5)
{
  unsigned char md5Buf[16];
  mbedtls_md5((const unsigned char *)buffer, bufLen, md5Buf);
  int i = 0;
    for(; i < 32; i+=2)
    {
    md5[i] = g_hex_hash[md5Buf[i >> 1] >> 4];
        md5[i+1] = g_hex_hash[md5Buf[i >> 1] & 0xF];
  }
}

int aos_base64_encode(const unsigned char *in, int inLen, char *out)
{
  static const char *ENC =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

  char *original_out = out;

  while (inLen) {
    // first 6 bits of char 1
    *out++ = ENC[*in >> 2];
    if (!--inLen) {
      // last 2 bits of char 1, 4 bits of 0
      *out++ = ENC[(*in & 0x3) << 4];
      *out++ = '=';
      *out++ = '=';
      break;
    }
    // last 2 bits of char 1, first 4 bits of char 2
    *out++ = ENC[((*in & 0x3) << 4) | (*(in + 1) >> 4)];
    in++;
    if (!--inLen) {
      // last 4 bits of char 2, 2 bits of 0
      *out++ = ENC[(*in & 0xF) << 2];
      *out++ = '=';
      break;
    }
    // last 4 bits of char 2, first 2 bits of char 3
    *out++ = ENC[((*in & 0xF) << 2) | (*(in + 1) >> 6)];
    in++;
    // last 6 bits of char 3
    *out++ = ENC[*in & 0x3F];
    in++, inLen--;
  }

  return (int)(out - original_out);
}

int signature_to_base64(const char * sig, int sigLen, const char * key, int keyLen, char * base64)
{
  unsigned char sha1Buf[20];
  hmac_sha1(sha1Buf, key, keyLen << 3, sig, sigLen << 3);
  return aos_base64_encode((const unsigned char *)sha1Buf, 20, base64);
}

#if defined(_WIN32)
static wchar_t *log_utf8_to_wchar(const char *src)
{
    if (!src)
    {
        return NULL;
    }
    int wchars_num = MultiByteToWideChar(CP_UTF8, 0, src, -1, 0, 0);
    wchar_t *output_buffer = (wchar_t *)malloc(wchars_num * sizeof(wchar_t));
    if (output_buffer == NULL)
    {
        aos_error_log("malloc memory failed, file path: %s", src);
        return NULL;
    }
    if (MultiByteToWideChar(CP_UTF8, 0, src, -1, output_buffer, wchars_num))
    {
        return output_buffer;
    }
    free(output_buffer);
    return NULL;
}

FILE *log_sys_fopen(const char *path, const char *mode)
{
    wchar_t *wpath = log_utf8_to_wchar(path);
    if (!wpath)
    {
        aos_error_log("Open file failed, invalid file path, not utf8: %s",
                      path);
        return NULL;
    }
    wchar_t *wmode = log_utf8_to_wchar(mode);
    if (!wmode)
    {
        aos_error_log("Open file failed, invalid file mode, not utf8: %s",
                      mode);
        free(wpath);
        return NULL;
    }
    FILE *result = _wfopen(wpath, wmode);
    if (result == NULL)
    {
        aos_error_log("Open file failed: %d", GetLastError());
    }
    free(wpath);
    free(wmode);
    return result;
}

int log_sys_open(const char *file, int flag, int mode)
{
    wchar_t *wfile = log_utf8_to_wchar(file);
    if (!wfile)
    {
        aos_error_log("Open file failed, invalid file path, not utf8: %s",
                      file);
        return -1;
    }
    int fd = _wopen(wfile, flag, mode);
    if (fd < 0)
    {
        aos_error_log("Open file failed: %d", GetLastError());
    }
    free(wfile);
    return fd;
}

int log_sys_open_sync(const char *file, int flag, int mode)
{
    wchar_t *wfile = log_utf8_to_wchar(file);
    if (!wfile)
    {
        aos_error_log("Open file failed, invalid file path, not utf8: %s",
                      file);
        return -1;
    }
    HANDLE h = CreateFileW(wfile, GENERIC_WRITE | GENERIC_READ, 0, NULL,
                           OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE)
    {
        aos_error_log("open sync file on win failed %s, error %d", file,
                      GetLastError());
        free(wfile);
        return -1;
    }
    int fd = _open_osfhandle((intptr_t)h, _O_RDWR);
    if (fd < 0)
    {
        aos_error_log("convert handle to fd failed %s, error %d", file,
                      GetLastError());
        CloseHandle(h);
        free(wfile);
        return -1;
    }
    free(wfile);
    return fd;
}

int log_sys_fsync(int fd)
{
    HANDLE h = (HANDLE)_get_osfhandle(fd);
    if (h == INVALID_HANDLE_VALUE)
    {
        aos_error_log("get file handle failure, %d", fd);
        return -1;
    }
    if (!FlushFileBuffers(h))
    {
        aos_error_log("flush file failure, fd:%d, err:%d", fd, GetLastError());
        return -1;
    }
    return 0;
}

void log_sleep_ms(int ms) { Sleep(ms); }
#else

FILE *log_sys_fopen(const char *path, const char *mode)
{
    return fopen(path, mode);
}

int log_sys_open(const char *file, int flag, int mode)
{
    return open(file, flag, mode);
}

int log_sys_open_sync(const char *file, int flag, int mode)
{
    return open(file, flag | O_SYNC, mode);
}

int log_sys_fsync(int fd) { return fsync(fd); }

void log_sleep_ms(int ms) { usleep(ms * 1000); }
#endif