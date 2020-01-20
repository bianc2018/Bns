/*
    通道基类
    一个实例代表一个数据通道
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

        //初始化
        virtual BNS_ERR_CODE init(const BnsPoint& local);

        //设置事件回调
        virtual BNS_ERR_CODE set_event_cb(BNS_EVENT_CB cb);

        //日志回调
        virtual BNS_ERR_CODE set_log_cb(BNS_LOG_CB cb);

        //buff申请回调
        virtual BNS_ERR_CODE set_shared_cb(BNS_MAKE_SHARED_CB cb);

        //buff申请回调
        virtual BNS_ERR_CODE set_recv_buff_size(size_t recv_buff_size);

        //激活
        virtual BNS_ERR_CODE active(const BnsPoint& remote);

        //发送数据
        virtual BNS_ERR_CODE send(std::shared_ptr<void> message, size_t message_size);

        //关闭一个通道
        virtual BNS_ERR_CODE close();

        //获取句柄
        virtual BNS_HANDLE get_handle();
    protected:
        //测试节点
        bool check_endpoint(const BnsPoint& point);
    protected:
        BNS_HANDLE handle_ ;
        BNS_EVENT_CB event_cb_;
        io_service &service_;

        //回调函数
        BNS_MAKE_SHARED_CB make_shared_;
        BNS_LOG_CB log_cb_;

        //接收数据缓冲区大小
        size_t recv_buff_size_;
    };
}
#endif