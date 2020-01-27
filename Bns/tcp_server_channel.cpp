#include "tcp_server_channel.h"

#include "tcp_client_channel.h"
BNS_ERR_CODE bns::TcpServerChannel::async_one_accept()
{
        //创建客户端
    auto cli = std::make_shared<TcpClientChannel>(service_);

    if (nullptr == cli)
    {
        PRINTFLOG(BL_DEBUG, "make_shared client error");
        EVENT_ERR_CB(BNS_NET_EVENT_TYPE::BNS_ACCEPT, BNS_ERR_CODE::BNS_ALLOC_FAIL);
        return BNS_ERR_CODE::BNS_ALLOC_FAIL;
    }

    cli->init(BNS_INVALID_POINT);
    cli->set_shared_cb(make_shared_);
    cli->set_log_cb(log_cb_);
    
    PRINTFLOG(BL_DEBUG, "%I64d accept one clent ", handle_);

    //异步接收连接
    auto self = shared_from_this();
    acceptor_.async_accept(cli->get_socket(), [this, self, cli]\
        (const boost::system::error_code& error)
        {
            if (error)
            {
                PRINTFLOG(BL_DEBUG,"async accept error what %s", error.message().c_str());

                EVENT_ERR_CB(BNS_NET_EVENT_TYPE::BNS_ACCEPT, BNS_ERR_CODE::BNS_ACCEPT_FAIL);
                return ;
            }
            else
            {
                if (cli->is_open())
                {
                    //协议栈异步处理连接
                    cli->active();
                    PRINTFLOG(BL_DEBUG, "%I64d accept a clent[%I64d] "\
                        , handle_,cli->get_handle());

                    accept_cb_(cli);
                    std::shared_ptr<BNS_HANDLE> h = std::make_shared<BNS_HANDLE>(cli->get_handle());
                    EVENT_CB(BNS_NET_EVENT_TYPE::BNS_ACCEPT,\
                        BNS_ERR_CODE::BNS_OK, h, sizeof(BNS_HANDLE));
                }
                else
                {
                    PRINTFLOG(BL_ERROR, "%I64d accept clent error,client is not open "\
                        , handle_);
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
    {
        PRINTFLOG(BL_ERROR, "ERROR,accept_cb_ is empty");
        return BNS_ERR_CODE::BNS_EMPYTY_ACCEPT_CB;
    }

    if (false == check_endpoint(local))
    {
        PRINTFLOG(BL_ERROR,"ERROR,POINT[%s:%d]", local.ip.c_str(), local.port);
        return BNS_ERR_CODE::BNS_ADDRESS_ENDPOINT_ERROR;
    }
    boost::system::error_code ec;
    boost::asio::ip::tcp::endpoint endpoint;

    try
    {
        boost::asio::ip::tcp::resolver resolver(service_);
        endpoint = *resolver.resolve(local.ip, std::to_string(local.port)).begin();
    }
    catch (const std::exception&e)
    {
        PRINTFLOG(BL_ERROR, "resolver error message=%s", e.what());
        return BNS_ERR_CODE::BNS_ADDRESS_ENDPOINT_ERROR;
    }
    
    //boost::asio::ip::tcp::endpoint endpoint(ip::make_address(local.ip), local.port);
    
    //打开连接
    acceptor_.open(endpoint.protocol(), ec);
    if (ec)
    {
        //printf("%s\n", ec.message().c_str());
        //LOG_ERR << "open error " << ec.value() << ec.message();
        PRINTFLOG(BL_ERROR,"acceptor open error local %s:%d what %s",\
            local.ip.c_str(), local.port,ec.message().c_str());
        return BNS_ERR_CODE::BNS_SOCKET_OPEN_FAIL;
    }
    PRINTFLOG(BL_INFO, "acceptor is opened");
    //设置参数，地址可重用
    acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true), ec);
    if (ec)
    {
        PRINTFLOG(BL_ERROR,"set_option reuse address error local %s:%d what %s", local.ip.c_str(),\
            local.port,ec.message().c_str());
        return BNS_ERR_CODE::BNS_SET_REUSE_ADDRESS_FAIL;
    }
    PRINTFLOG(BL_INFO, "acceptor is set reuse_address");
    //绑定地址
    acceptor_.bind(endpoint, ec);
    if (ec)
    {
        PRINTFLOG(BL_ERROR,"bind error local %s:%d what %s", local.ip.c_str(), \
            local.port, ec.message().c_str());
        return BNS_ERR_CODE::BNS_BIND_ADDRESS_FAIL;
    }
    PRINTFLOG(BL_INFO, "acceptor is bind local %s:%d", local.ip.c_str(), \
        local.port);
    //监听
    acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
    if (ec)
    {
        PRINTFLOG(BL_ERROR,"listen error local %s:%d what %s", local.ip.c_str(), \
            local.port, ec.message().c_str());
        return BNS_ERR_CODE::BNS_LISTEN_FAIL;
    }
    PRINTFLOG(BL_INFO, "acceptor is listen");
    return BNS_ERR_CODE::BNS_OK;
}

BNS_ERR_CODE bns::TcpServerChannel::active(const BnsPoint& remote)
{
    PRINTFLOG(BL_INFO, "[%I64d] start async_one_accept thread num=%d", handle_, accept_num_);
    for (auto i = 0; i < accept_num_; ++i)
    {
        auto ret = async_one_accept();
        if (BNS_ERR_CODE::BNS_OK != ret)
        {
            PRINTFLOG(BL_ERROR,"start async_one_accept error,[%I64d]", handle_);
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
            PRINTFLOG(BL_ERROR,"exception:%s", e.what());
            EVENT_ERR_CB(BNS_NET_EVENT_TYPE::BNS_CLOSED, BNS_ERR_CODE::BNS_NUKNOW_ERROR);
            return BNS_ERR_CODE::BNS_NUKNOW_ERROR;
        }
        PRINTFLOG(BL_INFO, "[%I64d] TcpServerChannel is closed", handle_);
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
