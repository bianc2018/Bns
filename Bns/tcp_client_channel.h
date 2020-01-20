/*
    tcp客户端通道

*/
#ifndef BNS_TCP_CLIENT_CHANNEL_H_
#define BNS_TCP_CLIENT_CHANNEL_H_
#include "channel.h"
#include <atomic>
namespace bns
{
	class TcpClientChannel:public Channel
	{
    public:
        TcpClientChannel( io_service& service);

        virtual ~TcpClientChannel();

        //初始化
        virtual BNS_ERR_CODE init(const BnsPoint& local);

        //激活
        virtual BNS_ERR_CODE active(const BnsPoint& remote);

        //发送数据
        virtual BNS_ERR_CODE send(std::shared_ptr<void> message, size_t message_size);

        //关闭一个通道
        virtual BNS_ERR_CODE close();

    public:
        //获取socket
        ip::tcp::socket& get_socket();

        //is open
        bool is_open();

        bool active();
    private:
        //异步发送数据
        BNS_ERR_CODE async_send(std::shared_ptr<void> buff, size_t buff_len, size_t beg = 0);

        //异步接收数据
        BNS_ERR_CODE async_recv(std::shared_ptr<void> buff = nullptr, size_t buff_len = 0);
    private:
        //是否链接？
        std::atomic_bool bconn_;

        ip::tcp::socket socket_;

	};
}
#endif
