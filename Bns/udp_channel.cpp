#include "udp_channel.h"

bns::UdpChannel::UdpChannel(io_service& service):
    Channel(service), socket_(service)
{
}

bns::UdpChannel::~UdpChannel()
{
    close();
}

BNS_ERR_CODE bns::UdpChannel::init(const BnsPoint& local)
{
    if (false == check_endpoint(local))
    {
        PRINTFLOG(BL_ERROR, "ERROR,POINT[%s:%d]", local.ip.c_str(), local.port);
        return BNS_ERR_CODE::BNS_ADDRESS_ENDPOINT_ERROR;
    }
    boost::system::error_code ec;
    boost::asio::ip::udp::endpoint endpoint;

    try
    {
        boost::asio::ip::udp::resolver resolver(service_);
        endpoint = *resolver.resolve(local.ip, std::to_string(local.port)).begin();
    }
    catch (const std::exception & e)
    {
        PRINTFLOG(BL_ERROR, "resolver error message=%s", e.what());
        return BNS_ERR_CODE::BNS_ADDRESS_ENDPOINT_ERROR;
    }

    //boost::asio::ip::tcp::endpoint endpoint(ip::make_address(local.ip), local.port);

    //打开连接
    socket_.open(endpoint.protocol(), ec);
    if (ec)
    {
        PRINTFLOG(BL_ERROR, "udp socket open error local %s:%d what %s", \
            local.ip.c_str(), local.port, ec.message().c_str());
        return BNS_ERR_CODE::BNS_SOCKET_OPEN_FAIL;
    }
    PRINTFLOG(BL_INFO, "udp socket is opened");
    
    //绑定地址
    socket_.bind(endpoint, ec);
    if (ec)
    {
        PRINTFLOG(BL_ERROR, "udp socket bind error local %s:%d what %s", local.ip.c_str(), \
            local.port, ec.message().c_str());
        return BNS_ERR_CODE::BNS_BIND_ADDRESS_FAIL;
    }
    PRINTFLOG(BL_INFO, "udp socket is bind local %s:%d", local.ip.c_str(), \
        local.port);
    return BNS_ERR_CODE::BNS_OK;
}

BNS_ERR_CODE bns::UdpChannel::active(const BnsPoint& remote)
{
    //绑定远程端点
    if (false == check_endpoint(remote))
    {
        PRINTFLOG(BL_ERROR, "ERROR,POINT[%s:%d]", remote.ip.c_str(), remote.port);
        return BNS_ERR_CODE::BNS_ADDRESS_ENDPOINT_ERROR;
    }

    boost::system::error_code ec;
    try
    {
        boost::asio::ip::udp::resolver resolver(service_);
        remote_point_ = *resolver.resolve(remote.ip, std::to_string(remote.port)).begin();
    }
    catch (const std::exception & e)
    {
        PRINTFLOG(BL_ERROR, "resolver error message=%s", e.what());
        return BNS_ERR_CODE::BNS_ADDRESS_ENDPOINT_ERROR;
    }

    //应该不用链接
    //异步读数据
    EVENT_OK_CB(BNS_NET_EVENT_TYPE::BNS_CONNECT_ESTABLISH);
    return async_recv();
}

BNS_ERR_CODE bns::UdpChannel::send(std::shared_ptr<void> message, size_t message_size)
{
    return async_send(message,message_size);
}

BNS_ERR_CODE bns::UdpChannel::close()
{
    if (socket_.is_open())
    {
        try
        {
            socket_.shutdown(socket_base::shutdown_send);
            socket_.shutdown(socket_base::shutdown_receive);

        }
        catch (std::exception & e)
        {
            PRINTFLOG(BL_ERROR, "exception:%s", e.what());
            EVENT_ERR_CB(BNS_NET_EVENT_TYPE::BNS_CLOSED, BNS_ERR_CODE::BNS_NUKNOW_ERROR);
            return BNS_ERR_CODE::BNS_NUKNOW_ERROR;
        }
        socket_.close();
        PRINTFLOG(BL_DEBUG, "UdpChannel[%I64d] closed ", handle_);
        EVENT_OK_CB(BNS_NET_EVENT_TYPE::BNS_CLOSED);
        return BNS_ERR_CODE::BNS_OK;
    }
    return BNS_ERR_CODE::BNS_EMPTY_SOCKET;
}

BNS_ERR_CODE bns::UdpChannel::async_send(std::shared_ptr<void> buff, size_t buff_len, size_t beg)
{
    //写完数据后的回调函数
    auto self = shared_from_this();
    auto send_handler = \
        [this, self, buff_len, beg, buff](boost::system::error_code ec, std::size_t s)
    {

        if (ec)
        {
            PRINTFLOG(BL_DEBUG, "async_send error what()=%s", ec.message().c_str());
            EVENT_ERR_CB(BNS_NET_EVENT_TYPE::BNS_SEND_COMPLETE, BNS_ERR_CODE::BNS_SEND_DATA_FAIL);
            //close();
            return;
        }
        //写完了
        if (buff_len <= s)
        {
            //返回异步写,取下一个buff
           // if (on_complate_)
            //    on_complate_(true);
            EVENT_SEND_OK_CB(buff, buff_len);
            return;
        }
        //未写完
        //计算剩下的字节
        int now_len = buff_len - s;
        PRINTFLOG(BL_DEBUG, "async_send  %p:len[%d]", buff.get(), s);
        //继续写
        async_send(buff, now_len, beg + s);
        return;
    };

    socket_.async_send_to(boost::asio::buffer((char*)buff.get() + beg, buff_len),remote_point_, \
        send_handler);
    return BNS_ERR_CODE::BNS_OK;
}

BNS_ERR_CODE bns::UdpChannel::async_recv(std::shared_ptr<void> buff, size_t buff_len)
{
    if (nullptr == buff)
    {
        buff_len = recv_buff_size_;
        buff = MAKE_SHARED(recv_buff_size_);
        if (nullptr == buff)
        {
            PRINTFLOG(BL_ERROR, "get cache error ch :%I64d", handle_);
            EVENT_ERR_CB(BNS_NET_EVENT_TYPE::BNS_RECV_DATA, BNS_ERR_CODE::BNS_ALLOC_FAIL);
            //close();
            return BNS_ERR_CODE::BNS_ALLOC_FAIL;
        }

    }
    auto self = shared_from_this();
    auto recv_handler = [this, self, buff, buff_len]\
        (boost::system::error_code ec, size_t recv_len)
    {
        if (ec)
        {
            //
            PRINTFLOG(BL_DEBUG, "async_read error what()=%s", ec.message().c_str());
            EVENT_ERR_CB(BNS_NET_EVENT_TYPE::BNS_RECV_DATA, BNS_ERR_CODE::BNS_RECV_DATA_FAIL);
            //close();
            return;
        }
        PRINTFLOG(BL_DEBUG, "async_read  %p:len[%d]", buff.get(), recv_len);
        EVENT_RECV_OK_CB(buff, recv_len);
        //重复接收数据
        async_recv(buff, buff_len);
    };
    
    //异步读数据
    socket_.async_receive(boost::asio::buffer(buff.get(), buff_len), recv_handler);
    return BNS_ERR_CODE::BNS_OK;
}
