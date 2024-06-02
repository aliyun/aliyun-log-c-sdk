#pragma warning(disable:4996)
#include "log_util.h"
#include "log_api.h"
#include "inner_log.h"

#include <string.h>
#include <stdio.h>
#include <windows.h>
#include <io.h>
#include <fcntl.h>

void test_fopen(const char* filename) {

    FILE *file;
    char *buffer;
    long fileLength;

    // 打开文件
    file = log_sys_fopen(filename, "b");  // 以二进制读模式打开
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }

    // 获取文件长度
    fseek(file, 0, SEEK_END);  // 将文件指针移动到文件的末尾
    fileLength = ftell(file);  // 获取当前文件指针的位置，即文件的长度
    rewind(file);  // 将文件指针重新指向文件开头

    // 分配内存
    buffer = (char *)malloc((fileLength + 1) * sizeof(char));  // 分配足够的内存存储整个文件，+1用于字符串终止符
    if (buffer == NULL) {
        perror("Error allocating memory");
        fclose(file);
        return 1;
    }

    // 读取文件内容到缓冲区
    fread(buffer, fileLength, 1, file);
    buffer[fileLength] = '\0';  // 添加字符串终止符

    // 打印文件内容
    printf("%s", buffer);

    // 关闭文件并释放内存
    fclose(file);
    free(buffer);
}

void test_open_write(const char *filename) {
    int fd; // 文件描述符

    // 打开文件，如果不存在则创建。使用 _O_WRONLY | _O_CREAT | _O_TRUNC 模式
    // _O_WRONLY：只写
    // _O_CREAT：不存在则创建
    // _O_TRUNC：如果文件已存在且为写模式打开，则其长度被截断为 0
    // _S_IREAD | _S_IWRITE：设置文件权限为可读写
    fd = log_sys_open(filename, O_RDWR|O_CREAT, 0644);
    if (fd == -1) {
        perror("Failed to open file");
        return 1;
    }
    const char *data = "Hello, world!\n";
      
    // 写入数据
    if (_write(fd, data, strlen(data)) == -1) {
        perror("Failed to write data");
        // 如果写入失败，尝试关闭文件并退出
        _close(fd);
        return 1;
    }

    printf("Data written to %s\n", filename);

    // 关闭文件
    _close(fd);

}

int main(int argc, char *argv[]) {
    aos_log_set_level(AOS_LOG_ALL);
    // a新建文件夹abC.abx.txt
    const char* name = "\xE6\xB5\x8B\xE8\xAF\x95\x61\x62\x63\x2E\x74\x78\x74";
    test_open_write(name);
    printf("ok write\n");
    test_fopen(name);
    printf("ok fopen\n");
    return 0;
}