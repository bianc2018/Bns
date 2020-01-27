#include "bns.h"

//事件回调
void server_event(BNS_HANDLE handle,
    BNS_NET_EVENT_TYPE type, BNS_ERR_CODE error_code, \
    std::shared_ptr<void> buff, size_t buff_len)
{
    if (BNS_ERR_CODE::BNS_OK != error_code)
    {
        printf("错误的发生:[%I64d] opt=%d,err=%d", handle, type, error_code);
        return;
    }
    if (BNS_NET_EVENT_TYPE::BNS_ACCEPT == type)
    {
        std::shared_ptr<BNS_HANDLE> cli = std::reinterpret_pointer_cast<BNS_HANDLE>(buff);
        BNS_HANDLE cli_handle = *cli;
        printf("accept...%d\n", (int)cli_handle);
        BNS_Send_String(cli_handle, "hello world:"+std::to_string(cli_handle));
        BNS_SetEvntCB(cli_handle, server_event);
        //std::string  hello = "hello world!";
        //std::shared_ptr<char> buff = std::make_shared<char>({})
        //BNS_Send(cli_handle)
    }
    else if(BNS_NET_EVENT_TYPE::BNS_RECV_DATA == type)
    {
        std::string str((char*)buff.get(), buff_len);
        printf("server BNS_RECV_DATA[%I64d] %d %s\n", (std::int64_t)handle, buff_len,str.c_str());
        BNS_Send_String(handle, str);
    }
}

int main()
{
    BNS_Init();
    BNS_HANDLE serv_handle;
    BnsPoint serv = { "0.0.0.0",2020 };
    auto ret = BNS_Add_Channnel(BNS_CHANNEL_TYPE::TCP_SERVER, serv, serv_handle);
    ret = BNS_SetEvntCB(serv_handle, server_event);
    ret = BNS_Connect(serv_handle, BNS_INVALID_POINT);

    system("pause");

    BNS_DInit();
    return 0;
}