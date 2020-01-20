#include "tcp_server_channel.h"

#include "tcp_client_channel.h"
BNS_ERR_CODE bns::TcpServerChannel::async_one_accept()
{
        //创建客户端
    auto cli = std::make_shared<TcpClientChannel>(service_);

    if (nullptr == cli)
    {
        EVENT_ERR_CB(BNS_NET_EVENT_TYPE::BNS_ACCEPT, BNS_ERR_CODE::BNS_ALLOC_FAIL);
        return BNS_ERR_CODE::BNS_ALLOC_FAIL;
    }

    cli->init(BNS_INVALID_POINT);
    cli->set_shared_cb(make_shared_);
    cli->set_log_cb(log_cb_);
    //cli->
    //异步接收连接
    auto self = shared_from_this();
    acceptor_.async_accept(cli->get_socket(), [this, self, cli]\
        (const boost::system::error_code& error)
        {
            if (error)
            {
                PRINTFLOG("async accept error what %s", error.message().c_str());

                EVENT_ERR_CB(BNS_NET_EVENT_TYPE::BNS_ACCEPT, BNS_ERR_CODE::BNS_ACCEPT_FAIL);
                return ;
            }
            else
            {
                if (cli->is_open())
                {
                    //协议栈异步处理连接
                    cli->active();
                    accept_cb_(cli);
                    std::shared_ptr<BNS_HANDLE> h = std::make_shared<BNS_HANDLE>(cli->get_handle());
                    EVENT_CB(BNS_NET_EVENT_TYPE::BNS_ACCEPT,\
                        BNS_ERR_CODE::BNS_OK, h, sizeof(BNS_HANDLE));
                }
                else
                {
                    //LOG_INFO << "获得连接：套接字错误 ";
                    //协议栈异步处理连接
                    EVENT_ERR_CB(BNS_NET_EVENT_TYPE::BNS_ACCEPT, \
                        BNS_ERR_CODE::BNS_ACCEPT_CLI_IS_NOT_OPEN);
                }

                //重复接收
                async_one_accept();

            }

        });
    return BNS_ERR_CODE::BNS_OK;
}


bns::TcpServerChannel::TcpServerChannel(io_service& service, bns::ACCEPT_CHANNEL_FN cb)
    :Channel(service),acceptor_(service),accept_num_(1), accept_cb_(cb)
{

}

bns::TcpServerChannel::~TcpServerChannel()
{
    close();
}

BNS_ERR_CODE bns::TcpServerChannel::init(const BnsPoint& local)
{
    if (!accept_cb_)
        return BNS_ERR_CODE::BNS_EMPYTY_ACCEPT_CB;

    if (false == check_endpoint(local))
    {
        PRINTFLOG("ERROR,POINT[%s:%d]", local.ip.c_str(), local.port);
        return BNS_ERR_CODE::BNS_ADDRESS_ENDPOINT_ERROR;
    }

    boost::asio::ip::tcp::endpoint endpoint(ip::make_address(local.ip), local.port);

    boost::system::error_code ec;
    //打开连接
    acceptor_.open(endpoint.protocol(), ec);
    if (ec)
    {
        printf("%s\n", ec.message().c_str());
        //LOG_ERR << "open error " << ec.value() << ec.message();
        PRINTFLOG("acceptor open error local %s:%d what %s",\
            local.ip.c_str(), local.port,ec.message().c_str());
        return BNS_ERR_CODE::BNS_SOCKET_OPEN_FAIL;
    }
    //设置参数，地址可重用
    acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true), ec);
    if (ec)
    {
        PRINTFLOG("set_option reuse address error local %s:%d what %s", local.ip.c_str(),\
            local.port,ec.message().c_str());
        return BNS_ERR_CODE::BNS_SET_REUSE_ADDRESS_FAIL;
    }
    //绑定地址
    acceptor_.bind(endpoint, ec);
    if (ec)
    {
        PRINTFLOG("bind error local %s:%d what %s", local.ip.c_str(), \
            local.port, ec.message().c_str());
        return BNS_ERR_CODE::BNS_BIND_ADDRESS_FAIL;
    }

    //监听
    acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
    if (ec)
    {
        PRINTFLOG("listen error local %s:%d what %s", local.ip.c_str(), \
            local.port, ec.message().c_str());
        return BNS_ERR_CODE::BNS_LISTEN_FAIL;
    }
    
    return BNS_ERR_CODE::BNS_OK;
}

BNS_ERR_CODE bns::TcpServerChannel::active(const BnsPoint& remote)
{
    for (auto i = 0; i < accept_num_; ++i)
    {
        auto ret = async_one_accept();
        if (BNS_ERR_CODE::BNS_OK != ret)
        {
            PRINTFLOG("start async_one_accept error,[%I64d]", handle_);
            return ret;
        }
    }
    return BNS_ERR_CODE::BNS_OK;
}

BNS_ERR_CODE bns::TcpServerChannel::close()
{
    if (acceptor_.is_open())
    {
        try
        {
            boost::system::error_code ec;
            acceptor_.cancel(ec);
            acceptor_.close(ec);

        }
        catch (std::exception & e)
        {
            PRINTFLOG("exception:%s", e.what());
            EVENT_ERR_CB(BNS_NET_EVENT_TYPE::BNS_CLOSED, BNS_ERR_CODE::BNS_NUKNOW_ERROR);
            return BNS_ERR_CODE::BNS_NUKNOW_ERROR;
        }
        
        EVENT_OK_CB(BNS_NET_EVENT_TYPE::BNS_CLOSED);
        return BNS_ERR_CODE::BNS_OK;
    }
    return BNS_ERR_CODE::BNS_EMPTY_SOCKET;
}

BNS_ERR_CODE bns::TcpServerChannel::set_accept_num(size_t num)
{
    if(num<=0)
        return BNS_ERR_CODE::BNS_PARAMS_IS_INVALID;
    accept_num_ = num;
    return BNS_ERR_CODE::BNS_OK;
}
