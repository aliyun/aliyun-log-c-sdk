# 程序可靠退出方案
## 简介
在使用日志服务C Producer时，若您的程序需要可靠退出，可以根据此文档介绍实现此功能。

## 问题背景
在程序退出时，C Producer会在给定时间范围内尽可能的将缓存中的日志发送出去，当等待超过`destroy_sender_wait_sec`若还没有发送成功，则会强制停止发送线程池。此时可能出现最后若干日志丢失的情况。

## 解决方法
针对以上问题，日志服务 C Producer提供了相应的数据导出以及导入接口，为应用程序在退出时提供日志数据可靠保存的方式。具体实现方式如下：
1. 在销毁Producer时，producer会将未发送完的数据通过发送回调函数`on_log_send_done`通知应用程序，错误码为`LOG_PRODUCER_SEND_EXIT_BUFFERED`
2. 应用程序在接收到此错误码时，将`log_bytes` `compressed_bytes` `buffer` 保存到本地
3. 在应用程序下一次启动时，读取本地文件，并调用`log_producer_client_add_raw_log_buffer`将上一次未发送完毕的数据发送出去

