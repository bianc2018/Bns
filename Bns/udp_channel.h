/*
    ����һ��udpͨ��ͨ��
    
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

        //��ʼ��
        virtual BNS_ERR_CODE init(const BnsPoint& local);

        //����
        virtual BNS_ERR_CODE active(const BnsPoint& remote);

        //��������
        virtual BNS_ERR_CODE send(std::shared_ptr<void> message, size_t message_size);

        //�ر�һ��ͨ��
        virtual BNS_ERR_CODE close();

    private:
        //�첽��������
        BNS_ERR_CODE async_send(std::shared_ptr<void> buff, size_t buff_len, size_t beg = 0);

        //�첽������
        BNS_ERR_CODE async_recv(std::shared_ptr<void> buff=nullptr, size_t buff_len=0);
    private:
        //�׽���
        ip::udp::socket socket_;

        //
        ip::udp::endpoint remote_point_;
    };
}

#endif