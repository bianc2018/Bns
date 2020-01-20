/*
    tcp�����
*/
#ifndef TCP_SERVER_CHANNEL_H_
#define TCP_SERVER_CHANNEL_H_
#include <functional>

#include "channel.h"

namespace bns
{
    //��������
    typedef std::function<BNS_ERR_CODE(std::shared_ptr<Channel> ch)> ACCEPT_CHANNEL_FN;

    class TcpServerChannel:public Channel
    {
    public:

        TcpServerChannel(io_service& service, bns::ACCEPT_CHANNEL_FN cb);

        virtual ~TcpServerChannel();

        //��ʼ��
        virtual BNS_ERR_CODE init(const BnsPoint& local);

        //����
        virtual BNS_ERR_CODE active(const BnsPoint& remote);

        //�ر�һ��ͨ��
        virtual BNS_ERR_CODE close();

        BNS_ERR_CODE set_accept_num(size_t num);

    private:
        //�첽��������
        //BNS_ERR_CODE async_accept(size_t accept_num = 1);

        //�첽��������
        BNS_ERR_CODE async_one_accept();

    private:
        //����������Ŀ
        size_t accept_num_;

        //add channel �����յ�ͨ���ӵ�ϵͳ��ȥ
        ACCEPT_CHANNEL_FN accept_cb_;

        ip::tcp::acceptor acceptor_;
    };
}
#endif//!TCP_SERVER_CHANNEL_H_