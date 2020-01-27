#include "tcp_client_channel.h"

bns::TcpClientChannel::TcpClientChannel(io_service& service)
    :Channel(service), socket_(service), bconn_(false)
{
}

bns::TcpClientChannel::~TcpClientChannel()
{
    close();
}

BNS_ERR_CODE bns::TcpClientChannel::init(const BnsPoint& local)
{
    if (INVAILD_HANDLE == handle_)
    {
        PRINTFLOG(BL_ERROR, "invaild handle");
        return BNS_ERR_CODE::BNS_INVAILD_HANDLE;
    }
    return BNS_ERR_CODE::BNS_OK;
}

//连接
BNS_ERR_CODE bns::TcpClientChannel::active(const BnsPoint& remote)
{
    if (false == check_endpoint(remote))
    {
        PRINTFLOG(BL_ERROR,"ERROR,POINT[%s:%d]", remote.ip.c_str(), remote.port);
        return BNS_ERR_CODE::BNS_ADDRESS_ENDPOINT_ERROR;
    }

    ip::tcp::endpoint remote_point(ip::address::from_string(remote.ip), remote.port);

    auto self = shared_from_this();
    auto con_handle = [this,self,remote](boost::system::error_code ec)
    {
        if (ec)
        {
            //减少打印
            PRINTFLOG(BL_DEBUG,"async_connect error what()=%s", ec.message().c_str());
            //上层关闭
            EVENT_ERR_CB(BNS_NET_EVENT_TYPE::BNS_CONNECT_ESTABLISH, BNS_ERR_CODE::BNS_CONNECT_FAIL);
            //close();
            return;
        }
        bconn_ = true;
        PRINTFLOG(BL_DEBUG, "tcp client[%I64d] is connected remote[%s:%d]", \
            get_handle(), remote.ip.c_str(), remote.port);
        EVENT_OK_CB(BNS_NET_EVENT_TYPE::BNS_CONNECT_ESTABLISH);
        //接收数据
        async_recv();
    };
    socket_.async_connect(remote_point, con_handle);
    PRINTFLOG(BL_DEBUG, "tcp client[%I64d] begin connect to remote[%s:%d]", \
        get_handle(), remote.ip.c_str(), remote.port);
    return BNS_ERR_CODE::BNS_OK;
}

BNS_ERR_CODE bns::TcpClientChannel::send(std::shared_ptr<void> message, size_t message_size)
{
   return async_send(message, message_size);
}

BNS_ERR_CODE bns::TcpClientChannel::close()
{
    if (bconn_)
    {
        try
        {
            socket_.shutdown(socket_base::shutdown_send);
            socket_.shutdown(socket_base::shutdown_receive);
            
        }
        catch (std::exception & e)
        {
            PRINTFLOG(BL_ERROR,"exception:%s",e.what());
            EVENT_ERR_CB(BNS_NET_EVENT_TYPE::BNS_CLOSED, BNS_ERR_CODE::BNS_NUKNOW_ERROR);
            return BNS_ERR_CODE::BNS_NUKNOW_ERROR;
        }
        socket_.close();
        bconn_ = false;
        PRINTFLOG(BL_DEBUG, "TcpClientChannel[%I64d] closed ", handle_);
        EVENT_OK_CB(BNS_NET_EVENT_TYPE::BNS_CLOSED);
        return BNS_ERR_CODE::BNS_OK;
    }
    return BNS_ERR_CODE::BNS_EMPTY_SOCKET;
}

boost::asio::ip::tcp::socket& bns::TcpClientChannel::get_socket()
{
    return socket_;
}

bool bns::TcpClientChannel::is_open()
{
    //套接字以打开
    return socket_.is_open();
}

bool bns::TcpClientChannel::active()
{
    //链接已建立
    bconn_ = true;
    //bconn_ = true;
    //EVENT_OK_CB(BNS_NET_EVENT_TYPE::BNS_CONNECT_ESTABLISH);
    //接收数据
    async_recv();
    return true;
}

BNS_ERR_CODE bns::TcpClientChannel::async_send(std::shared_ptr<void> buff,\
    size_t buff_len, size_t beg)
{
    if (false == bconn_)
    {
        EVENT_ERR_CB(BNS_NET_EVENT_TYPE::BNS_SEND_COMPLETE, BNS_ERR_CODE::BNS_CHANNEL_NOT_CONN);
        return BNS_ERR_CODE::BNS_CHANNEL_NOT_CONN;
    }
    //写完数据后的回调函数
    auto self = shared_from_this();
    auto send_handler = \
        [this, self, buff_len, beg, buff](boost::system::error_code ec, std::size_t s)
    {

        if (ec)
        {
            PRINTFLOG(BL_DEBUG,"async_send error what()=%s", ec.message().c_str());
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
            EVENT_SEND_OK_CB(buff,buff_len);
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

    socket_.async_write_some(boost::asio::buffer((char*)buff.get() + beg, buff_len),\
        send_handler);
    return BNS_ERR_CODE::BNS_OK;
}

BNS_ERR_CODE bns::TcpClientChannel::async_recv(std::shared_ptr<void> buff, size_t buff_len)
{
    if (false == bconn_)
    {
        PRINTFLOG(BL_ERROR, "TcpClientChannel[%I64d] is not connected ", handle_);
        EVENT_ERR_CB(BNS_NET_EVENT_TYPE::BNS_RECV_DATA, BNS_ERR_CODE::BNS_CHANNEL_NOT_CONN);
        return BNS_ERR_CODE::BNS_CHANNEL_NOT_CONN;
    }

    if (nullptr == buff)
    {
        buff_len = recv_buff_size_;
        buff = MAKE_SHARED(recv_buff_size_);
        if (nullptr == buff)
        {
            PRINTFLOG(BL_ERROR,"get cache error ch :%I64d", handle_);
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
            PRINTFLOG(BL_DEBUG,"async_read error what()=%s", ec.message().c_str());
            EVENT_ERR_CB(BNS_NET_EVENT_TYPE::BNS_RECV_DATA, BNS_ERR_CODE::BNS_RECV_DATA_FAIL);
            //close();
            return;
        }
        PRINTFLOG(BL_DEBUG, "async_read  %p:len[%d]", buff.get(), recv_len);
        EVENT_RECV_OK_CB(buff, recv_len);
        //重复接收数据
        async_recv(buff, buff_len);
    };
    socket_.async_read_some(boost::asio::buffer(buff.get(), buff_len), recv_handler);
    return BNS_ERR_CODE::BNS_OK;
}
