#include "bns.h"
/*
    客户端
*/

void add(BNS_EVENT_CB cb)
{
    BnsPoint dst = { "127.0.0.1",2020 };
    BNS_HANDLE cli_handle;
    auto ret = BNS_Add_Channnel(BNS_CHANNEL_TYPE::TCP_CLIENT, BNS_INVALID_POINT, \
        cli_handle);
    ret = BNS_SetEvntCB(cli_handle, cb);
    ret = BNS_Connect(cli_handle, dst);
}

//事件回调
void client_event(BNS_HANDLE handle,
    BNS_NET_EVENT_TYPE type, BNS_ERR_CODE error_code, \
    std::shared_ptr<void> buff, size_t buff_len)
{
    if (BNS_ERR_CODE::BNS_OK != error_code)
    {
        printf("错误的发生:[%I64d] opt=%d,err=%d", handle, type, error_code);
        return;
    }

    if (BNS_NET_EVENT_TYPE::BNS_CONNECT_ESTABLISH == type)
    {
        printf("connect...[%I64d]\n", handle);
        add(client_event);
        //BNS_Send_String(handle, "hello world");
    }
    else if (BNS_NET_EVENT_TYPE::BNS_RECV_DATA == type)
    {
        std::string str((char*)buff.get(), buff_len);
        printf("client BNS_RECV_DATA[%I64d] %d %s\n", (std::int64_t)handle, buff_len, str.c_str());
        //BNS_Send_String(handle, str );
        BNS_Close(handle);
    }
}

int main()
{
    //auto ret = BNS_SetLogCB(nullptr);
    BNS_Init();
    
    add(client_event);
    system("pause");

    BNS_DInit();
    return 0;
}