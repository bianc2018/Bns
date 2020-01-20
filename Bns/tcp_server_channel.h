/*
    tcp服务端
*/
#ifndef TCP_SERVER_CHANNEL_H_
#define TCP_SERVER_CHANNEL_H_
#include <functional>

#include "channel.h"

namespace bns
{
    //接收链接
    typedef std::function<BNS_ERR_CODE(std::shared_ptr<Channel> ch)> ACCEPT_CHANNEL_FN;

    class TcpServerChannel:public Channel
    {
    public:

        TcpServerChannel(io_service& service, bns::ACCEPT_CHANNEL_FN cb);

        virtual ~TcpServerChannel();

        //初始化
        virtual BNS_ERR_CODE init(const BnsPoint& local);

        //激活
        virtual BNS_ERR_CODE active(const BnsPoint& remote);

        //关闭一个通道
        virtual BNS_ERR_CODE close();

        BNS_ERR_CODE set_accept_num(size_t num);

    private:
        //异步接收链接
        //BNS_ERR_CODE async_accept(size_t accept_num = 1);

        //异步接收链接
        BNS_ERR_CODE async_one_accept();

    private:
        //接收请求数目
        size_t accept_num_;

        //add channel 将接收的通道加到系统中去
        ACCEPT_CHANNEL_FN accept_cb_;

        ip::tcp::acceptor acceptor_;
    };
}
#endif//!TCP_SERVER_CHANNEL_H_