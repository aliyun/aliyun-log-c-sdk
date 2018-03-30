#ifndef LOG_INNER_INCLUDE_H
#define LOG_INNER_INCLUDE_H

//操作系统头文件
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <errno.h>

#ifdef WIN32
#define inline __inline
#include <winsock2.h>
#include <sys/timeb.h>
#include <windows.h>
#include <process.h>
#include <assert.h>

typedef int socklen_t;



#elif defined(_VXWORKS)

#include <vxworks.h>
#include <netdb.h>
#include <string.h>
#include <ctype.h>
#include <types.h>
#include <sockLib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/times.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <hostLib.h>
#include <ticklib.h>
#include <drv/timer/ppcDecTimer.h>

#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/ioctl.h>
#include  <net/if.h>
#include  <net/if_arp.h>
#include <tipc/tipc.h>
#include <taskLib.h>
#include <selectLib.h>//for vx6
#include <ioLib.h>
#include <ioctl.h>

#else

#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <semaphore.h>
#include <assert.h>
#include <sys/time.h>
#include <stdint.h>

#endif


#if defined WIN32 || defined _VXWORKS

typedef unsigned char u_char;
typedef unsigned char u_int8;
typedef unsigned short u_int16;
typedef unsigned int u_int32;
typedef unsigned __int64 u_int64;
typedef signed char int8;
typedef signed short int16;
typedef signed int int32;
typedef signed __int64 int64;

#elif defined __linux__

typedef unsigned char u_char;
typedef unsigned char u_int8;
typedef unsigned short u_int16;
typedef unsigned int u_int32;
typedef unsigned long long u_int64;
typedef signed char int8;
typedef signed short int16;
typedef signed int int32;
typedef long long int64;

typedef int SOCKET;
#define SOCKET_ERROR -1
#define INVALID_SOCKET -1
#define __in   // 参数输入
#define __out  // 参数输出
#define closesocket close
#define stricmp strcasecmp
typedef int BOOL;
#define TRUE 1
#define FALSE 0

#define Sleep(param) usleep(1000*(param))
#define strcpy_s(a, b, c) strcpy(a, c)
#define sprintf_s(a, b, c) sprintf(a, c)
#define strncpy_s(a, b, c, d) strncpy(a, c, d)
#define vsprintf_s(a, b, c, d) vsprintf(a, c, d)
#define _strdup strdup
#define _stricmp stricmp


#endif

#endif //LOG_INNER_INCLUDE_H
