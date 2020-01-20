#include "bns_define.h"
#include "bns_service.h"

#include <functional>

#include "tcp_client_channel.h"
#include "tcp_server_channel.h"

using namespace bns;

bns::BnsService::BnsService()\
    :service_(),work_(service_),\
    run_flag_(true),thread_num_(get_default_thread_num()),
    make_shared_(default_make_shared), log_cb_(default_log_print),
    recv_buff_size(1024*1024*5)
{

}

bns::BnsService::~BnsService()
{
    BNS_DInit();
}

BNS_ERR_CODE bns::BnsService::BNS_Init(int thread_num)
{
    if (thread_num > 0)
    {
        thread_num_ = thread_num;
    }

    for (size_t i = 0; i < thread_num_; ++i)
    {
        thread_vec_.push_back(std::thread(&BnsService::run_worker, this));
    }

    return BNS_ERR_CODE::BNS_OK;
}

BNS_ERR_CODE bns::BnsService::BNS_DInit()
{
    //防止重复释放
    if (run_flag_)
    {
        run_flag_ = false;
        
        //删除释放所有的设备
        std::lock_guard<std::mutex> lk(bns_channel_map_lock_);
        for (auto p = bns_channel_map_.begin(); p != bns_channel_map_.end();)
        {
            auto  ch = p->second;
            if (ch)
            {
                ch->close();
            }
        
            p = bns_channel_map_.erase(p);
        }
        //重置
        service_.stop();
        service_.reset();
        //释放
        work_.~work();
        for (auto& t : thread_vec_)
        {
            if (t.joinable())
                t.join();
        }
    }
    return BNS_ERR_CODE::BNS_OK;
}

BNS_ERR_CODE bns::BnsService::BNS_Add_Channnel(BNS_CHANNEL_TYPE type, const BnsPoint& local, BNS_HANDLE& handle)
{
    //创建套接字
    std::shared_ptr<Channel>ch = nullptr;
    if (BNS_CHANNEL_TYPE::TCP_CLIENT == type)
    {
        ch = std::make_shared<TcpClientChannel>(service_);
    }
    else if (BNS_CHANNEL_TYPE::TCP_SERVER == type)
    {
        ch = std::make_shared<TcpServerChannel>(service_,\
            std::bind(&BnsService::add_channnel,this,std::placeholders::_1));
    }
    else
    {
        return BNS_ERR_CODE::BNS_NO_SUPPORT;
    }
    
    if (nullptr == ch)
        return BNS_ERR_CODE::BNS_CHANNEL_CREATE_FAIL;
    
    ch->set_log_cb(std::bind(&BnsService::printf_log, this, std::placeholders::_1));
    ch->set_shared_cb(std::bind(&BnsService::get_cache, this, std::placeholders::_1));

    auto err = ch->init(local);
    
    if (BNS_ERR_CODE::BNS_OK != err)
        return err;

    handle = ch->get_handle();
    return add_channnel(ch);
}

BNS_ERR_CODE bns::BnsService::BNS_Connect(BNS_HANDLE handle, const BnsPoint& remote)
{
    auto ch = get_channel(handle);
    if (ch)
    {
        return ch->active(remote);
    }
    return BNS_ERR_CODE::BNS_INVAILD_HANDLE;
}

BNS_ERR_CODE bns::BnsService::BNS_Close(BNS_HANDLE handle)
{
    auto ch = get_channel(handle);
    if (nullptr == ch)
    {
        PRINTFLOG("ch[%lld] is no exist!", handle);
        return BNS_ERR_CODE::BNS_INVAILD_HANDLE;
    }

    return ch->close();
}

BNS_ERR_CODE bns::BnsService::BNS_Send(BNS_HANDLE handle,\
    std::shared_ptr<char> message, size_t message_size)
{
    auto ch = get_channel(handle);
    if (nullptr == ch)
    {
        PRINTFLOG("ch[%ll] is no exist!", handle);
        return BNS_ERR_CODE::BNS_INVAILD_HANDLE;
    }
    return ch->send(message, message_size);
}

BNS_ERR_CODE bns::BnsService::BNS_Send_String(BNS_HANDLE handle, const std::string& message)
{
    if (message.empty())
        return BNS_ERR_CODE::BNS_OK;
    auto message_size = message.size();
    auto cache = get_cache(message_size);
    if (nullptr == cache)
        return BNS_ERR_CODE::BNS_ALLOC_FAIL;
    memcpy(cache.get(), message.c_str(), message_size);
    return BNS_Send(handle, cache, message_size);
}

BNS_ERR_CODE bns::BnsService::BNS_SetEvntCB(BNS_HANDLE handle, BNS_EVENT_CB cb)
{
    auto ch = get_channel(handle);
    if (nullptr == ch)
    {
        PRINTFLOG("ch[%I64d] is no exist!", handle);
        return BNS_ERR_CODE::BNS_INVAILD_HANDLE;
    }
    return ch->set_event_cb(cb);
}

BNS_ERR_CODE bns::BnsService::BNS_SetLogCB(BNS_LOG_CB cb)
{
    if (cb)
    {
        log_cb_ = cb;
        return BNS_ERR_CODE::BNS_OK;
    }
    return BNS_ERR_CODE::BNS_EMPYTY_LOG_CB;
}

BNS_ERR_CODE bns::BnsService::BNS_SetMakeSharedCB( BNS_MAKE_SHARED_CB cb)
{
    if (cb)
    {
        make_shared_ = cb;
        return BNS_ERR_CODE::BNS_OK;
    }
    return BNS_ERR_CODE::BNS_EMPYTY_MAKE_SHARED_CB;
}


std::shared_ptr<Channel> bns::BnsService::get_channel(BNS_HANDLE handle)
{
    std::lock_guard<std::mutex> lk(bns_channel_map_lock_);
    auto p = bns_channel_map_.find(handle);
    if (bns_channel_map_.end() == p)
        return nullptr;
    return p->second;
}

//BNS_ERR_CODE bns::BnsService::async_accept(std::shared_ptr<Channel> ch, size_t accept_num)
//{
//    if (nullptr == ch || ch->type != BNS_CHANNEL_TYPE::TCP_SERVER || nullptr == ch->socket)
//        return BNS_ERR_CODE::BNS_NO_SUPPORT;
//    for (size_t i = 0; i < accept_num; ++i)
//        async_one_accept(ch);
//    return BNS_ERR_CODE::BNS_OK;
//}
//
//BNS_ERR_CODE bns::BnsService::async_one_accept(std::shared_ptr<Channel> ch)
//{
//    //创建客户端
//    std::shared_ptr<BnsChannel> cli \
//        = std::make_shared<BnsChannel>();
//
//    if (nullptr == cli)
//    {
//        if (ch->event_cb)
//            ch->event_cb(ch->handle,
//                BNS_NET_EVENT_TYPE::BNS_ACCEPT, BNS_ERR_CODE::BNS_ALLOC_FAIL, nullptr, 0);
//        return BNS_ERR_CODE::BNS_ALLOC_FAIL;
//    }
//
//    cli->type = BNS_CHANNEL_TYPE::TCP_CLIENT;
//    cli->handle = generate_handle();
//    if (INVAILD_HANDLE == ch->handle)
//    {
//        if (ch->event_cb)
//            ch->event_cb(ch->handle,
//                BNS_NET_EVENT_TYPE::BNS_ACCEPT, BNS_ERR_CODE::BNS_INVAILD_HANDLE, nullptr, 0);
//        return BNS_ERR_CODE::BNS_INVAILD_HANDLE;
//    }
//
//    if (nullptr != get_channel(cli->handle))
//    {
//        if (ch->event_cb)
//            ch->event_cb(ch->handle,
//                BNS_NET_EVENT_TYPE::BNS_ACCEPT, BNS_ERR_CODE::BNS_HANDLE_IS_USED, nullptr, 0);
//        return BNS_ERR_CODE::BNS_HANDLE_IS_USED;
//    }
//    
//    cli->socket = std::make_shared<ip::tcp::acceptor>(service_);
//    if (nullptr == cli->socket)
//    {
//        if (ch->event_cb)
//            ch->event_cb(ch->handle,
//                BNS_NET_EVENT_TYPE::BNS_ACCEPT, BNS_ERR_CODE::BNS_CREATE_SOCKET_FAIL, nullptr, 0);
//        return BNS_ERR_CODE::BNS_CREATE_SOCKET_FAIL;
//    }
//       
//    auto accept = TCP_SRV_CAST(ch);
//    //异步接收连接
//    accept->async_accept(*(TCP_CLI_CAST(cli)), [this,ch, cli]\
//        (const boost::system::error_code& error)
//        {
//            if (error)
//            {
//                PRINTFLOG("async accept error what %s", error.message().c_str());
//
//                if (ch->event_cb)
//                    ch->event_cb(ch->handle,
//                        BNS_NET_EVENT_TYPE::BNS_ACCEPT, BNS_ERR_CODE::BNS_ACCEPT_FAIL, nullptr, 0);
//                return ;
//            }
//            else
//            {
//                if (TCP_CLI_CAST(cli)->is_open())
//                {
//                    //协议栈异步处理连接
//                    add_channnel(cli);
//                    std::shared_ptr<BNS_HANDLE> h = std::make_shared<BNS_HANDLE>(cli->handle);
//                    if (ch->event_cb)
//                        ch->event_cb(ch->handle,
//                            BNS_NET_EVENT_TYPE::BNS_ACCEPT,
//                            BNS_ERR_CODE::BNS_OK, h, sizeof(BNS_HANDLE));
//                }
//                else
//                {
//                    //LOG_INFO << "获得连接：套接字错误 ";
//                    //协议栈异步处理连接
//
//                    if (ch->event_cb)
//                        ch->event_cb(ch->handle,
//                            BNS_NET_EVENT_TYPE::BNS_ACCEPT,
//                            BNS_ERR_CODE::BNS_ACCEPT_CLI_IS_NOT_OPEN, nullptr, 0);
//                    
//                }
//
//                //重复接收
//                async_one_accept(ch);
//
//            }
//
//        });
//    return BNS_ERR_CODE::BNS_OK;
//}
//
//BNS_ERR_CODE bns::BnsService::bind_address(std::shared_ptr<Channel> ch,\
//    const std::string& local_ip, int local_port)
//{
//    if (nullptr == ch || ch->type != BNS_CHANNEL_TYPE::TCP_SERVER || nullptr == ch->socket)
//        return BNS_ERR_CODE::BNS_NO_SUPPORT;
//
//    auto acceptor = TCP_SRV_CAST(ch);
//    if(nullptr == acceptor)
//        return BNS_ERR_CODE::BNS_CHANNEL_TYPE_FAIL;
//
//    //boost::asio::ip::tcp::resolver resolver(service_);
//    boost::asio::ip::tcp::endpoint endpoint(ip::make_address(local_ip), local_port);
//
//    boost::system::error_code ec;
//    //打开连接
//    acceptor->open(endpoint.protocol(), ec);
//    if (ec)
//    {
//        printf("%s\n", ec.message().c_str());
//        //LOG_ERR << "open error " << ec.value() << ec.message();
//        PRINTFLOG("acceptor open error local %s:%d what %s",\
//            local_ip.c_str(), local_port,ec.message().c_str());
//        return BNS_ERR_CODE::BNS_SOCKET_OPEN_FAIL;
//    }
//    //设置参数，地址可重用
//    acceptor->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true), ec);
//    if (ec)
//    {
//        PRINTFLOG("set_option reuse address error local %s:%d what %s", local_ip.c_str(),\
//            local_port,ec.message().c_str());
//        return BNS_ERR_CODE::BNS_SET_REUSE_ADDRESS_FAIL;
//    }
//    //绑定地址
//    acceptor->bind(endpoint, ec);
//    if (ec)
//    {
//        PRINTFLOG("bind error local %s:%d what %s", local_ip.c_str(), \
//            local_port, ec.message().c_str());
//        return BNS_ERR_CODE::BNS_BIND_ADDRESS_FAIL;
//    }
//
//    //监听
//    acceptor->listen(boost::asio::socket_base::max_listen_connections, ec);
//    if (ec)
//    {
//        PRINTFLOG("listen error local %s:%d what %s", local_ip.c_str(), \
//            local_port, ec.message().c_str());
//        return BNS_ERR_CODE::BNS_LISTEN_FAIL;
//    }
//    
//    return BNS_ERR_CODE::BNS_OK;
//}

void bns::BnsService::run_worker()
{
    service_.run();
}

void bns::BnsService::printf_log(const std::string& message)
{
    if (log_cb_)
        log_cb_(message);
}

std::shared_ptr<char> bns::BnsService::get_cache(size_t size)
{
    if (make_shared_)
        return make_shared_(size);
    return nullptr;
}

size_t bns::BnsService::get_default_thread_num()
{
    return std::thread::hardware_concurrency()*2+2;
}

BNS_ERR_CODE bns::BnsService::add_channnel(std::shared_ptr<Channel> ch)
{
    std::lock_guard<std::mutex> lk(bns_channel_map_lock_);
    bns_channel_map_[ch->get_handle()] = ch;
    return BNS_ERR_CODE::BNS_OK;
}

