/*
    表征一个udp通信通道
    
*/
#ifndef UDP_CHANNEL_H_
#define UDP_CHANNEL_H_
#include "channel.h"
namespace bns
{
    class UdpChannel :public Channel
    {
    public:
        UdpChannel(io_service& service);

        virtual ~UdpChannel();

        //初始化
        virtual BNS_ERR_CODE init(const BnsPoint& local);

        //激活
        virtual BNS_ERR_CODE active(const BnsPoint& remote);

        //发送数据
        virtual BNS_ERR_CODE send(std::shared_ptr<void> message, size_t message_size);

        //关闭一个通道
        virtual BNS_ERR_CODE close();

    private:
        //异步发送数据
        BNS_ERR_CODE async_send(std::shared_ptr<void> buff, size_t buff_len, size_t beg = 0);

        //异步读数据
        BNS_ERR_CODE async_recv(std::shared_ptr<void> buff=nullptr, size_t buff_len=0);
    private:
        //套接字
        ip::udp::socket socket_;

        //
        ip::udp::endpoint remote_point_;
    };
}

#endif