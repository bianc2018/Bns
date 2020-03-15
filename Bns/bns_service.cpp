#include "bns_define.h"
#include "bns_service.h"

#include <functional>

#include "tcp_client_channel.h"
#include "tcp_server_channel.h"
#include "udp_channel.h"

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
    PRINTFLOG(BL_DEBUG, "BnsService init thread_num=%d", thread_num_);
    for (size_t i = 0; i < thread_num_; ++i)
    {
        PRINTFLOG(BL_DEBUG, "BnsService  run_worker %d", i);
        thread_vec_.push_back(std::thread(&BnsService::run_worker, this));
    }
    PRINTFLOG(BL_DEBUG, "BnsService init ok");
    return BNS_ERR_CODE::BNS_OK;
}

BNS_ERR_CODE bns::BnsService::BNS_DInit()
{
    //防止重复释放
    if (run_flag_)
    {
        run_flag_ = false;
        
        //删除释放所有的设备
        {
            std::lock_guard<std::mutex> lk(bns_channel_map_lock_);
            PRINTFLOG(BL_DEBUG, "BnsService release all channel size=%d", bns_channel_map_.size());
            for (auto p = bns_channel_map_.begin(); p != bns_channel_map_.end();)
            {
                auto  ch = p->second;
                if (ch)
                {
                    PRINTFLOG(BL_DEBUG, "BnsService close channel[%I64d]", p->first);
                    ch->close();
                }
                else
                {
                    PRINTFLOG(BL_WRAN, "BnsService close channel[%I64d] is nullptr",p->first);
                }

                p = bns_channel_map_.erase(p);
            }
        }
        //重置
        service_.stop();
        service_.reset();
        
        PRINTFLOG(BL_DEBUG, "BnsService asio service is stoped");

        //释放
        work_.~work();
        PRINTFLOG(BL_DEBUG, "BnsService asio work is release");

        for (auto& t : thread_vec_)
        {
            if (t.joinable())
                t.join();
        }
        PRINTFLOG(BL_DEBUG, "all work thread is join !");
    }
    PRINTFLOG(BL_DEBUG, "BnsService is release !");
    return BNS_ERR_CODE::BNS_OK;
}

BNS_ERR_CODE bns::BnsService::BNS_Add_Channnel(BNS_CHANNEL_TYPE type, \
    const BnsPoint& local, BNS_HANDLE& handle)
{
    PRINTFLOG(BL_INFO, "add a channel type=%d local=%s:%d",type,local.ip.c_str(),local.port);
    //创建套接字
    std::shared_ptr<Channel>ch = nullptr;
    if (BNS_CHANNEL_TYPE::TCP_CLIENT == type)
    {
        ch = std::make_shared<TcpClientChannel>(service_);
        if(ch)
            PRINTFLOG(BL_DEBUG, "new a TcpClientChannel handle[%I64d]", ch->get_handle());
    }
    else if (BNS_CHANNEL_TYPE::TCP_SERVER == type)
    {
        ch = std::make_shared<TcpServerChannel>(service_,\
            std::bind(&BnsService::add_channnel,this,std::placeholders::_1));
        if(ch)
            PRINTFLOG(BL_DEBUG, "new a TcpServerChannel handle[%I64d]", ch->get_handle());
    }
    else if (BNS_CHANNEL_TYPE::UDP == type)
    {
        ch = std::make_shared<UdpChannel>(service_);
        if (ch)
            PRINTFLOG(BL_DEBUG, "new a UdpChannel handle[%I64d]", ch->get_handle());
    }
    else
    {
        PRINTFLOG(BL_ERROR, "no support type=%d", type);
        return BNS_ERR_CODE::BNS_NO_SUPPORT;
    }
    
    if (nullptr == ch)
    {
        PRINTFLOG(BL_ERROR, "channel cannt create  type=%d", type);
        return BNS_ERR_CODE::BNS_CHANNEL_CREATE_FAIL;
    }
    
    PRINTFLOG(BL_DEBUG, "channel init ");

    ch->set_log_cb(std::bind(&BnsService::printf_log, \
        this, std::placeholders::_1, std::placeholders::_2));
    ch->set_shared_cb(std::bind(&BnsService::get_cache, this, std::placeholders::_1));

    auto err = ch->init(local);
    
    if (BNS_ERR_CODE::BNS_OK != err)
    {
        PRINTFLOG(BL_ERROR, "channel[%I64d] init fail ret=%d",ch->get_handle(), err);
        return err;
    }

    handle = ch->get_handle();
    PRINTFLOG(BL_DEBUG, "try add channel[%I64d] ",handle);
    return add_channnel(ch);
}

BNS_ERR_CODE bns::BnsService::BNS_Active(BNS_HANDLE handle, const BnsPoint& remote)
{
    auto ch = get_channel(handle);
    if (ch)
    {
        PRINTFLOG(BL_INFO, "active channel handle=%I64d remote=%s:%d", \
            handle, remote.ip.c_str(), remote.port);
        return ch->active(remote);
    }
    PRINTFLOG(BL_ERROR, "channel handle=%I64d not find,this handle is invaild", \
        handle);
    return BNS_ERR_CODE::BNS_INVAILD_HANDLE;
}

BNS_ERR_CODE bns::BnsService::BNS_Close(BNS_HANDLE handle)
{
    auto ch = get_channel(handle);
    if (nullptr == ch)
    {
        PRINTFLOG(BL_ERROR,"ch[%lld] is no exist!", handle);
        return BNS_ERR_CODE::BNS_INVAILD_HANDLE;
    }
    {
        std::lock_guard<std::mutex> lk(bns_channel_map_lock_);
        bns_channel_map_.erase(handle);
    }
    PRINTFLOG(BL_INFO, "close channel handle=%I64d",handle);
    return ch->close();
}

BNS_ERR_CODE bns::BnsService::BNS_Send(BNS_HANDLE handle,\
    std::shared_ptr<char> message, size_t message_size)
{
    auto ch = get_channel(handle);
    if (nullptr == ch)
    {
        PRINTFLOG(BL_ERROR, "ch[%lld] is no exist!", handle);
        return BNS_ERR_CODE::BNS_INVAILD_HANDLE;
    }
    PRINTFLOG(BL_INFO, "channel[%I64d] send data size:%u",handle, message_size);
    return ch->send(message, message_size);
}

BNS_ERR_CODE bns::BnsService::BNS_Send_String(BNS_HANDLE handle, const std::string& message)
{
    if (message.empty())
    {
        PRINTFLOG(BL_WRAN, "channel[%I64d] send empty message", handle);
        return BNS_ERR_CODE::BNS_OK;
    }
    auto message_size = message.size();
    auto cache = get_cache(message_size);
    if (nullptr == cache)
    {
        PRINTFLOG(BL_ERROR, "channel[%I64d] get_cache error,size=%d", handle,message_size);
        return BNS_ERR_CODE::BNS_ALLOC_FAIL;
    }
    memcpy(cache.get(), message.c_str(), message_size);
    return BNS_Send(handle, cache, message_size);
}

BNS_ERR_CODE bns::BnsService::BNS_SetEvntCB(BNS_HANDLE handle, BNS_EVENT_CB cb)
{
    auto ch = get_channel(handle);
    if (nullptr == ch)
    {
        PRINTFLOG(BL_ERROR,"ch[%I64d] is no exist!", handle);
        return BNS_ERR_CODE::BNS_INVAILD_HANDLE;
    }
    //注册信息
    return ch->set_event_cb([this,cb](BNS_HANDLE handle, \
        BNS_NET_EVENT_TYPE type, BNS_ERR_CODE error_code, \
        std::shared_ptr<void> buff, size_t buff_len) 
        {
            //发生错误就关闭
            if (BNS_ERR_CODE::BNS_OK != error_code &&
                BNS_NET_EVENT_TYPE::BNS_CLOSED != type)
            {
                PRINTFLOG(BL_ERROR, "event_cb ch[%I64d] error,opt=%d,error=%d try to close!"\
                    , handle,type,error_code);
                BNS_Close(handle);
            }

            //可以统计用

            //真正调用函数
            if (cb)
                cb(handle, type, error_code, buff, buff_len);
        });
}

BNS_ERR_CODE bns::BnsService::BNS_SetLogCB(BNS_LOG_CB cb)
{
    log_cb_ = cb;
    return BNS_ERR_CODE::BNS_OK;
   /* if (cb)
    {
        log_cb_ = cb;
        return BNS_ERR_CODE::BNS_OK;
    }
    PRINTFLOG(BL_ERROR, "log_cb_ dont set,log_cb_ is empty!");
    return BNS_ERR_CODE::BNS_EMPYTY_LOG_CB;*/
}

BNS_ERR_CODE bns::BnsService::BNS_SetMakeSharedCB( BNS_MAKE_SHARED_CB cb)
{
    if (cb)
    {
        make_shared_ = cb;
        return BNS_ERR_CODE::BNS_OK;
    }
    PRINTFLOG(BL_ERROR, "make_shared_ dont set,make_shared_ is empty!");
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
    PRINTFLOG(BL_DEBUG, "run_worker is start!");
    service_.run();
    PRINTFLOG(BL_DEBUG, "run_worker is end!");
}

void bns::BnsService::printf_log(BLOG_LEVEL lv, const std::string& message)
{
    if (log_cb_)
        log_cb_(lv,message);
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
    if (false == run_flag_)
    {
        PRINTFLOG(BL_ERROR, "channel[%I64d] is cannt add, service is exit", ch->get_handle());
        return BNS_ERR_CODE::BNS_SERVICE_IS_STOPED;
    }
    std::lock_guard<std::mutex> lk(bns_channel_map_lock_);
    bns_channel_map_[ch->get_handle()] = ch;
    PRINTFLOG(BL_DEBUG, "channel[%I64d] is added", ch->get_handle());
    return BNS_ERR_CODE::BNS_OK;
}

