/*
    bns 内部定义
*/
#ifndef BNS_DEFINE_H_
#define BNS_DEFINE_H_

#include "bns.h"
#include <mutex>

#define PRINTF_BUFF_SIZE 2048
static std::mutex PRINTFLOG_LOCK;
static char PRINTFLOG_MESSAGE[PRINTF_BUFF_SIZE] = { 0 };
static char PRINTFLOG_DST[PRINTF_BUFF_SIZE] = { 0 };
//日志打印
#define PRINTFLOG(l,fmt,...) do\
{\
    std::lock_guard<std::mutex> lk(PRINTFLOG_LOCK);\
    snprintf(PRINTFLOG_MESSAGE,PRINTF_BUFF_SIZE,fmt,##__VA_ARGS__);\
\
    auto  now_time = time(nullptr);\
    auto file = std::string(__FILE__);\
    auto p = file.find_last_of("/\\");\
    auto file_name = file.substr(p + 1);\
    auto time_str = ctime(&now_time);\
\
    time_str[strlen(time_str)-1]='\0';\
\
    snprintf(PRINTFLOG_DST, PRINTF_BUFF_SIZE, \
    "%s [%s][%d]:%s\n", \
    time_str, \
    file_name.c_str(), \
    __LINE__, PRINTFLOG_MESSAGE);\
\
    if (log_cb_)\
        log_cb_(l,PRINTFLOG_DST);\
}while(false)\

#define MAKE_SHARED(x) make_shared_?make_shared_(x):nullptr

//回调
#define EVENT_CB(opt,err,data,data_len) \
if (event_cb_)\
    event_cb_(handle_,opt,err,data,data_len)

#define EVENT_RECV_OK_CB(buff,len) \
EVENT_CB(BNS_NET_EVENT_TYPE::BNS_RECV_DATA, BNS_ERR_CODE::BNS_OK, buff, len)

#define EVENT_SEND_OK_CB(buff,len) \
EVENT_CB(BNS_NET_EVENT_TYPE::BNS_SEND_COMPLETE, BNS_ERR_CODE::BNS_OK, buff, len)

#define EVENT_ERR_CB(opt,err) \
EVENT_CB(opt, err, nullptr, 0)

#define EVENT_OK_CB(opt) \
EVENT_CB(opt, BNS_ERR_CODE::BNS_OK, nullptr, 0)

#ifndef INVAILD_HANDLE
#define INVAILD_HANDLE BNS_HANDLE(-1)
#endif

namespace bns
{
    //默认的函数
    static std::shared_ptr<char> default_make_shared(size_t memory_size)
    {
        return std::shared_ptr<char>(new char[memory_size] {0}, std::default_delete<char[]>());
    }
    static std::string g_log_tag[4] = { "DEBUG","INFO","WRAN","ERROR" };
    static void default_log_print(BLOG_LEVEL lv,const std::string& log_message)
    {
        auto str = g_log_tag[lv] +":"+ log_message;
        printf(str.c_str());
    }
    //句柄生成
    static BNS_HANDLE  g_handle = INVAILD_HANDLE;
    static std::mutex  g_handle_lock;
    static BNS_HANDLE generate_handle()
    {
        std::lock_guard<std::mutex> lk(g_handle_lock);
        ++g_handle;
        if (g_handle < 0)
            g_handle = 0;
        return g_handle;
    }

}
#endif