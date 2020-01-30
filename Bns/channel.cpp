#include "channel.h"
#include "bns_define.h"
bns::Channel::Channel(io_service& service)\
    :handle_(generate_handle()), service_(service), recv_buff_size_(10*1024)
{

}

bns::Channel::~Channel()
{
}

BNS_ERR_CODE bns::Channel::init(const BnsPoint& local)
{
    PRINTFLOG(BL_DEBUG,"this channel no support :init");
    return BNS_ERR_CODE::BNS_NO_SUPPORT;
}

BNS_ERR_CODE bns::Channel::set_event_cb(BNS_EVENT_CB cb)
{
    event_cb_ = cb;
    return BNS_ERR_CODE::BNS_OK;
}

BNS_ERR_CODE bns::Channel::set_log_cb(BNS_LOG_CB cb)
{
    log_cb_ = cb;
    return BNS_ERR_CODE::BNS_OK;
}

BNS_ERR_CODE bns::Channel::set_shared_cb(BNS_MAKE_SHARED_CB cb)
{
    make_shared_ = cb;
    return BNS_ERR_CODE::BNS_OK;
}

BNS_ERR_CODE bns::Channel::set_recv_buff_size(size_t recv_buff_size)
{
    recv_buff_size_ = recv_buff_size;
    return BNS_ERR_CODE::BNS_OK;
}

BNS_ERR_CODE bns::Channel::active(const BnsPoint& remote)
{
    PRINTFLOG(BL_DEBUG,"this channel no support :active");
    return BNS_ERR_CODE::BNS_NO_SUPPORT;
}

BNS_ERR_CODE bns::Channel::send(std::shared_ptr<void> message, size_t message_size)
{
    PRINTFLOG(BL_DEBUG, "this channel no support :send");
    return BNS_ERR_CODE::BNS_NO_SUPPORT;
}

BNS_ERR_CODE bns::Channel::close()
{
    PRINTFLOG(BL_DEBUG, "this channel no support :close");
    return BNS_ERR_CODE::BNS_NO_SUPPORT;
}

BNS_HANDLE bns::Channel::get_handle()
{
    return handle_;
}

bool bns::Channel::check_endpoint(const BnsPoint& point)
{
    return "" != point.ip&&point.port>=0;
}
