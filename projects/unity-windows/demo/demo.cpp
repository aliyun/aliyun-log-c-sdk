// demo.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <Windows.h>
#include "aliyun_log.h"


void demo_log_send_done_function(const char* config_name, uint32_t result, size_t log_bytes, size_t compressed_bytes, const char* error_message, void* user_param)
{
    std::cout << "demo_log_send_done_function: result: " << result << ", id: " << user_param << "\n";
    if (6 == result)
    {
        AliyunLog* aliyun_log = (AliyunLog*)user_param;
        aliyun_log_set_accesskey(aliyun_log, "", "", "");
    }
}


int main()
{
    std::cout << "Hello World!\n";

    AliyunLog* aliyun_log = aliyun_log_create(
        "https://cn-beijing.log.aliyuncs.com", 
        "", 
        "", 
        "", 
        "", 
        NULL
    );

    std::cout << "create aliyun log. id: " << aliyun_log << "\n";

    aliyun_log_set_send_done_function(demo_log_send_done_function);

    aliyun_log_set_project(aliyun_log, "demo-for-mobile-apm");
    aliyun_log_set_logstore(aliyun_log, "test");

    uint32_t length = 2;
    char** key_values = (char**)malloc(sizeof(char*) * length * 2);
    
    key_values[0] = const_cast<char*>("key1");
    key_values[1] = const_cast<char*>("value1");
    key_values[2] = const_cast<char*>("key2");
    key_values[3] = const_cast<char*>("value2");

    int result = aliyun_log_add_log(aliyun_log, length, key_values);
    std::cout << "add log result: " << result << "\n";
    
    Sleep(15000);

    aliyun_log_destroy(aliyun_log);
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
