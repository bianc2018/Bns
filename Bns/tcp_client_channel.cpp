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
    if(INVAILD_HANDLE ==  handle_)
        return BNS_ERR_CODE::BNS_INVAILD_HANDLE;
    return BNS_ERR_CODE::BNS_OK;
}

//����
BNS_ERR_CODE bns::TcpClientChannel::active(const BnsPoint& remote)
{
    if (false == check_endpoint(remote))
    {
        PRINTFLOG("ERROR,POINT[%s:%d]", remote.ip.c_str(), remote.port);
        return BNS_ERR_CODE::BNS_ADDRESS_ENDPOINT_ERROR;
    }

    ip::tcp::endpoint remote_point(ip::address::from_string(remote.ip), remote.port);

    auto self = shared_from_this();
    auto con_handle = [this,self](boost::system::error_code ec)
    {
        if (ec)
        {
            PRINTFLOG("async_connect error what()=%s", ec.message().c_str());
            EVENT_ERR_CB(BNS_NET_EVENT_TYPE::BNS_CONNECT_ESTABLISH, BNS_ERR_CODE::BNS_CONNECT_FAIL);
            close();
            return;
        }
        bconn_ = true;
        EVENT_OK_CB(BNS_NET_EVENT_TYPE::BNS_CONNECT_ESTABLISH);
        //��������
        async_recv();
    };
    socket_.async_connect(remote_point, con_handle);
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
            PRINTFLOG("exception:%s",e.what());
            EVENT_ERR_CB(BNS_NET_EVENT_TYPE::BNS_CLOSED, BNS_ERR_CODE::BNS_NUKNOW_ERROR);
            return BNS_ERR_CODE::BNS_NUKNOW_ERROR;
        }
        socket_.close();
        bconn_ = false;
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
    //�׽����Դ�
    return socket_.is_open();
}

bool bns::TcpClientChannel::active()
{
    //�����ѽ���
    bconn_ = true;
    //bconn_ = true;
    //EVENT_OK_CB(BNS_NET_EVENT_TYPE::BNS_CONNECT_ESTABLISH);
    //��������
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
    //д�����ݺ�Ļص�����
    auto self = shared_from_this();
    auto send_handler = \
        [this, self, buff_len, beg, buff](boost::system::error_code ec, std::size_t s)
    {

        if (ec)
        {
            PRINTFLOG("async_send error what()=%s", ec.message().c_str());
            EVENT_ERR_CB(BNS_NET_EVENT_TYPE::BNS_SEND_COMPLETE, BNS_ERR_CODE::BNS_SEND_DATA_FAIL);
            close();
            return;
        }
        //д����
        if (buff_len <= s)
        {
            //�����첽д,ȡ��һ��buff
           // if (on_complate_)
            //    on_complate_(true);
            EVENT_SEND_OK_CB(buff,buff_len);
            return;
        }
        //δд��
        //����ʣ�µ��ֽ�
        int now_len = buff_len - s;
        //����д
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
        EVENT_ERR_CB(BNS_NET_EVENT_TYPE::BNS_RECV_DATA, BNS_ERR_CODE::BNS_CHANNEL_NOT_CONN);
        return BNS_ERR_CODE::BNS_CHANNEL_NOT_CONN;
    }

    if (nullptr == buff)
    {
        buff_len = recv_buff_size_;
        buff = MAKE_SHARED(recv_buff_size_);
        if (nullptr == buff)
        {
            PRINTFLOG("get cache error ch :%I64d", handle_);
            EVENT_ERR_CB(BNS_NET_EVENT_TYPE::BNS_RECV_DATA, BNS_ERR_CODE::BNS_ALLOC_FAIL);
            close();
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
            PRINTFLOG("async_read error what()=%s", ec.message().c_str());
            EVENT_ERR_CB(BNS_NET_EVENT_TYPE::BNS_RECV_DATA, BNS_ERR_CODE::BNS_RECV_DATA_FAIL);
            close();
            return;
        }
        EVENT_RECV_OK_CB(buff, recv_len);
        //�ظ���������
        async_recv(buff, buff_len);
    };
    socket_.async_read_some(boost::asio::buffer(buff.get(), buff_len), recv_handler);
    return BNS_ERR_CODE::BNS_OK;
}
