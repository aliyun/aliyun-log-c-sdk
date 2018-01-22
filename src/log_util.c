#include <arpa/inet.h>
#include <string.h>
#include "log_util.h"
#include "md5.h"
#include "hmac-sha.h"

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
