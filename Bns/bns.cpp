#include "bns.h"

#include "bns_service.h"
static bns::BnsService gService;

EXPORT_SYMBOLS BNS_ERR_CODE BNS_Init(int thread_num)
{
    return gService.BNS_Init(thread_num);
}

EXPORT_SYMBOLS BNS_ERR_CODE BNS_DInit()
{
    return gService.BNS_DInit();
}

EXPORT_SYMBOLS BNS_ERR_CODE BNS_Add_Channnel(BNS_CHANNEL_TYPE type,\
    const BnsPoint& local, BNS_HANDLE& handle)
{
    return gService.BNS_Add_Channnel(type,local, handle);
}

EXPORT_SYMBOLS BNS_ERR_CODE BNS_Active(BNS_HANDLE handle, const BnsPoint& remote)
{
    return gService.BNS_Active(handle, remote);
}

EXPORT_SYMBOLS BNS_ERR_CODE BNS_Close(BNS_HANDLE handle)
{
    return gService.BNS_Close(handle);
}

EXPORT_SYMBOLS BNS_ERR_CODE BNS_Send(BNS_HANDLE handle, std::shared_ptr<char> message, size_t message_size)
{
    return gService.BNS_Send(handle, message, message_size);
}

EXPORT_SYMBOLS BNS_ERR_CODE BNS_Send_String(BNS_HANDLE handle, const std::string& message)
{
    return gService.BNS_Send_String(handle, message);
}

EXPORT_SYMBOLS BNS_ERR_CODE BNS_SetEvntCB(BNS_HANDLE handle, BNS_EVENT_CB cb)
{
    return gService.BNS_SetEvntCB(handle, cb);
}

EXPORT_SYMBOLS BNS_ERR_CODE BNS_SetLogCB(BNS_LOG_CB cb)
{
    return gService.BNS_SetLogCB(cb);
}

EXPORT_SYMBOLS BNS_ERR_CODE BNS_SetMakeSharedCB(BNS_MAKE_SHARED_CB cb)
{
    return gService.BNS_SetMakeSharedCB(cb);
}
