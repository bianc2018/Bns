/*  
    ��������ͨ�ſ����
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
    //����
    class BnsService
    {
    public:
        BnsService();
        ~BnsService();
    public:
        //�����ӿ�
        //��ʼ��
        BNS_ERR_CODE BNS_Init(int thread_num = 0);

        //����ʼ��
        BNS_ERR_CODE BNS_DInit();

        //����ͨ�����Զ���������
        BNS_ERR_CODE BNS_Add_Channnel(BNS_CHANNEL_TYPE type\
            , const BnsPoint& local,
            BNS_HANDLE& handle);

        ////��������
        BNS_ERR_CODE BNS_Active(BNS_HANDLE handle,
            const BnsPoint& remoter);

        //�ر�һ��ͨ��
        BNS_ERR_CODE BNS_Close(BNS_HANDLE handle);

        //��������
        BNS_ERR_CODE BNS_Send(BNS_HANDLE handle, \
            std::shared_ptr<char> message, size_t message_size);

        BNS_ERR_CODE BNS_Send_String(BNS_HANDLE handle, const std::string& message);

        //�����¼��ص�
        BNS_ERR_CODE BNS_SetEvntCB(BNS_HANDLE handle, BNS_EVENT_CB cb);

        //������־�ص�
        BNS_ERR_CODE BNS_SetLogCB( BNS_LOG_CB cb);

        //���û���ص�
        BNS_ERR_CODE BNS_SetMakeSharedCB(BNS_MAKE_SHARED_CB cb);

    public:
        
        std::shared_ptr<Channel> get_channel(BNS_HANDLE handle);

        ////�첽��������
        //BNS_ERR_CODE async_accept(std::shared_ptr<Channel> ch,size_t accept_num=1);

        ////�첽��������
        //BNS_ERR_CODE async_one_accept(std::shared_ptr<Channel> ch);

        ////�󶨵�ַ
        //BNS_ERR_CODE bind_address(std::shared_ptr<Channel> ch,
        //    const std::string& local_ip, int local_port);

        //������
        void run_worker();

        //��ӡ��־
        void printf_log(BLOG_LEVEL lv,const std::string & message);

        //���뻺����
        std::shared_ptr<char> get_cache(size_t size);

        //��ȡĬ�ϵ��߳���
        size_t get_default_thread_num();

        //����tcp�ͻ���
        //����ͨ�����Զ���������
        BNS_ERR_CODE add_channnel(std::shared_ptr<Channel> ch);
    private:
        //ͨ����
        std::mutex bns_channel_map_lock_;
        //bns_channel_map_;
        std::unordered_map<BNS_HANDLE, std::shared_ptr<Channel> > bns_channel_map_;

        //����
        io_service service_;
        io_service::work work_;

        //�����߳�
        std::atomic_bool run_flag_;
        int thread_num_;
        std::vector<std::thread> thread_vec_;

        //�ص�����
        BNS_MAKE_SHARED_CB make_shared_;
        BNS_LOG_CB log_cb_;

        //�������ݻ�������С
        size_t recv_buff_size;
    };
}
#endif
