/*  
    基础网络通信库服务
    hql 2020/01/04
*/
#ifndef BNS_SERVCIE_
#define BNS_SERVCIE_

#include <unordered_map>
#include <mutex>
#include <thread>
#include <atomic>

//#define 
#include "channel.h"

namespace bns
{
    //服务
    class BnsService
    {
    public:
        BnsService();
        ~BnsService();
    public:
        //公共接口
        //初始化
        BNS_ERR_CODE BNS_Init(int thread_num = 0);

        //反初始化
        BNS_ERR_CODE BNS_DInit();

        //增加通道，自动建立链接
        BNS_ERR_CODE BNS_Add_Channnel(BNS_CHANNEL_TYPE type\
            , const BnsPoint& local,
            BNS_HANDLE& handle);

        ////建立链接
        BNS_ERR_CODE BNS_Active(BNS_HANDLE handle,
            const BnsPoint& remoter);

        //关闭一个通道
        BNS_ERR_CODE BNS_Close(BNS_HANDLE handle);

        //发送数据
        BNS_ERR_CODE BNS_Send(BNS_HANDLE handle, \
            std::shared_ptr<char> message, size_t message_size);

        BNS_ERR_CODE BNS_Send_String(BNS_HANDLE handle, const std::string& message);

        //设置事件回调
        BNS_ERR_CODE BNS_SetEvntCB(BNS_HANDLE handle, BNS_EVENT_CB cb);

        //设置日志回调
        BNS_ERR_CODE BNS_SetLogCB( BNS_LOG_CB cb);

        //设置缓存回调
        BNS_ERR_CODE BNS_SetMakeSharedCB(BNS_MAKE_SHARED_CB cb);

    public:
        
        std::shared_ptr<Channel> get_channel(BNS_HANDLE handle);

        ////异步接收链接
        //BNS_ERR_CODE async_accept(std::shared_ptr<Channel> ch,size_t accept_num=1);

        ////异步接收链接
        //BNS_ERR_CODE async_one_accept(std::shared_ptr<Channel> ch);

        ////绑定地址
        //BNS_ERR_CODE bind_address(std::shared_ptr<Channel> ch,
        //    const std::string& local_ip, int local_port);

        //运行期
        void run_worker();

        //打印日志
        void printf_log(BLOG_LEVEL lv,const std::string & message);

        //申请缓冲区
        std::shared_ptr<char> get_cache(size_t size);

        //获取默认的线程数
        size_t get_default_thread_num();

        //增加tcp客户端
        //增加通道，自动建立链接
        BNS_ERR_CODE add_channnel(std::shared_ptr<Channel> ch);
    private:
        //通道表
        std::mutex bns_channel_map_lock_;
        //bns_channel_map_;
        std::unordered_map<BNS_HANDLE, std::shared_ptr<Channel> > bns_channel_map_;

        //服务
        io_service service_;
        io_service::work work_;

        //运行线程
        std::atomic_bool run_flag_;
        int thread_num_;
        std::vector<std::thread> thread_vec_;

        //回调函数
        BNS_MAKE_SHARED_CB make_shared_;
        BNS_LOG_CB log_cb_;

        //接收数据缓冲区大小
        size_t recv_buff_size;
    };
}
#endif
