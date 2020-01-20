/*
    ͨ������
    һ��ʵ������һ������ͨ��
    hql 2020 01 15
*/
#ifndef BNS_CHANNEL_H_
#define BNS_CHANNEL_H_
#include "bns_define.h"
#include <boost/asio.hpp>

namespace bns
{
    using namespace boost::asio;
    class Channel :public std::enable_shared_from_this<Channel>
    {
    public:
        Channel(io_service& service);

        virtual ~Channel();

        //��ʼ��
        virtual BNS_ERR_CODE init(const BnsPoint& local);

        //�����¼��ص�
        virtual BNS_ERR_CODE set_event_cb(BNS_EVENT_CB cb);

        //��־�ص�
        virtual BNS_ERR_CODE set_log_cb(BNS_LOG_CB cb);

        //buff����ص�
        virtual BNS_ERR_CODE set_shared_cb(BNS_MAKE_SHARED_CB cb);

        //buff����ص�
        virtual BNS_ERR_CODE set_recv_buff_size(size_t recv_buff_size);

        //����
        virtual BNS_ERR_CODE active(const BnsPoint& remote);

        //��������
        virtual BNS_ERR_CODE send(std::shared_ptr<void> message, size_t message_size);

        //�ر�һ��ͨ��
        virtual BNS_ERR_CODE close();

        //��ȡ���
        virtual BNS_HANDLE get_handle();
    protected:
        //���Խڵ�
        bool check_endpoint(const BnsPoint& point);
    protected:
        BNS_HANDLE handle_ ;
        BNS_EVENT_CB event_cb_;
        io_service &service_;

        //�ص�����
        BNS_MAKE_SHARED_CB make_shared_;
        BNS_LOG_CB log_cb_;

        //�������ݻ�������С
        size_t recv_buff_size_;
    };
}
#endif