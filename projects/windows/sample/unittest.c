#pragma warning(disable : 4996)
#include "inner_log.h"
#include "log_api.h"
#include "log_util.h"

#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

void delete_file(const char *filename)
{
    int wchars_num = MultiByteToWideChar(CP_UTF8, 0, filename, -1, 0, 0);
    wchar_t output_buffer[4096];
    memset(output_buffer, 0, sizeof(output_buffer));
    MultiByteToWideChar(CP_UTF8, 0, filename, -1, output_buffer, wchars_num);
    DeleteFileW(output_buffer);
    printf("file deleted\n");
}

void fopen_write(const char *filename)
{
    FILE *file = log_sys_fopen(filename, "wb"); // 以二进制写模式打开
    if (file == NULL)
    {
        perror("Error opening file");
        return 1;
    }
    const char *data = "Hello, world! fopen_write\n";
    if (fwrite(data, strlen(data), 1, file) != 1)
    {
        perror("Error writing to file");
        fclose(file);
        return 1;
    }
    printf("Data written\n");
    fclose(file);
}

void fopen_read(const char *filename)
{
    char buffer[4096];
    long fileLength;

    FILE *file = log_sys_fopen(filename, "rb"); // 以二进制读模式打开
    if (file == NULL)
    {
        perror("Error opening file");
        return 1;
    }

    // 获取文件长度
    fseek(file, 0, SEEK_END);
    fileLength = ftell(file);
    rewind(file);

    fread(buffer, fileLength, 1, file);
    buffer[fileLength] = '\0';

    printf("%s\n", buffer);
    fclose(file);
}

void open_write(const char *filename)
{
    int fd;
    fd = log_sys_open(filename, O_RDWR | O_CREAT, 0644);
    if (fd == -1)
    {
        perror("Failed to open file");
        return 1;
    }
    const char *data = "Hello, world! open_write\n";

    if (_write(fd, data, strlen(data)) == -1)
    {
        perror("Failed to write data");
        _close(fd);
        return 1;
    }

    printf("Data written\n");
    _close(fd);
}

void open_write_sync(const char *filename)
{
    int fd;
    fd = log_sys_open_sync(filename, O_RDWR | O_CREAT, 0644);
    if (fd == -1)
    {
        perror("Failed to open file");
        return 1;
    }
    const char *data = "Hello, world! open_write_sync\n";

    if (_write(fd, data, strlen(data)) == -1)
    {
        perror("Failed to write data");
        _close(fd);
        return 1;
    }

    printf("Data written\n");
    _close(fd);
}

void open_read(const char *filename)
{
    int fd;
    fd = log_sys_open(filename, O_RDWR | O_CREAT, 0644);
    if (fd == -1)
    {
        perror("Failed to open file");
        return 1;
    }
    char buffer[4096];
    memset(buffer, 0, sizeof(buffer));
    if (_read(fd, buffer, sizeof(buffer)) == -1)
    {
        perror("Failed to read data");
        _close(fd);
        return 1;
    }
    printf("%s\n", buffer);
    _close(fd);
}

int main(int argc, char *argv[])
{
    aos_log_set_level(AOS_LOG_ALL);
    // 新建文件夹abc.txt
    const char *name = "\xE6\xB5\x8B\xE8\xAF\x95\x61\x62\x63\x2E\x74\x78\x74";
    fopen_write(name);
    fopen_read(name);
    open_read(name);
    delete_file(name);

    open_write_sync(name);
    fopen_read(name);
    open_read(name);
    open_write_sync(name);
    open_write(name);
    fopen_write(name);
    fopen_read(name);
    open_read(name);
    delete_file(name);
    return 0;
}