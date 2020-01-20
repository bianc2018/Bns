/*
    tcp�ͻ���ͨ��

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

        //��ʼ��
        virtual BNS_ERR_CODE init(const BnsPoint& local);

        //����
        virtual BNS_ERR_CODE active(const BnsPoint& remote);

        //��������
        virtual BNS_ERR_CODE send(std::shared_ptr<void> message, size_t message_size);

        //�ر�һ��ͨ��
        virtual BNS_ERR_CODE close();

    public:
        //��ȡsocket
        ip::tcp::socket& get_socket();

        //is open
        bool is_open();

        bool active();
    private:
        //�첽��������
        BNS_ERR_CODE async_send(std::shared_ptr<void> buff, size_t buff_len, size_t beg = 0);

        //�첽��������
        BNS_ERR_CODE async_recv(std::shared_ptr<void> buff = nullptr, size_t buff_len = 0);
    private:
        //�Ƿ����ӣ�
        std::atomic_bool bconn_;

        ip::tcp::socket socket_;

	};
}
#endif
