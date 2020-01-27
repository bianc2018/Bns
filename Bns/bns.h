/*
    ����boost.asio ��װ�Ļ��������
    hql 2020/01/04
*/
#ifndef BNS_H_
#define BNS_H_

#include <memory>
#include <functional>
#include <string>

//���嵼������
#ifdef _WIN32
#define EXPORT_SYMBOLS extern "C" __declspec(dllexport)
#else
#define EXPORT_SYMBOLS 
#endif

//�����¼��ص�����
enum class BNS_NET_EVENT_TYPE
{
    //����ȷ�� establish 
    BNS_CONNECT_ESTABLISH = 1,
    //���յ�����
    BNS_ACCEPT ,
    //���յ�����
    BNS_RECV_DATA ,
    //������������ complete
    BNS_SEND_COMPLETE,
    //�����Ѿ��ر�
    BNS_CLOSED,
};
//������
enum class BNS_ERR_CODE
{
    //�޴���
    BNS_OK,
    //�ڴ�����ʧ��
    BNS_ALLOC_FAIL = -998,
    //ͨ�����Ͳ���
    BNS_CHANNEL_TYPE_FAIL,
    //��Ч�ľ��
    BNS_INVAILD_HANDLE,
    //�ظ��ľ��
    BNS_HANDLE_IS_USED,
    //�����׽���ʧ��
    BNS_CREATE_SOCKET_FAIL,
    //����ʧ��
    BNS_CONNECT_FAIL,
    //��������ʧ��
    BNS_RECV_DATA_FAIL,
    //��������ʧ��
    BNS_SEND_DATA_FAIL,
    //�յ��¼��ص�
    BNS_EMPYTY_EVENT_CB,
    //�յ���־�ص�
    BNS_EMPYTY_LOG_CB,
    //�յ��ڴ�����ص�
    BNS_EMPYTY_MAKE_SHARED_CB,
    //�յĽ��պ���
    BNS_EMPYTY_ACCEPT_CB,
    //�յ�BUFF
    BNS_EMPYTY_BUFF,
    //��ʧ��
    BNS_SOCKET_OPEN_FAIL,
    //���õ�ַʧ��
    BNS_SET_REUSE_ADDRESS_FAIL,
    //�󶨵�ַʧ��
    BNS_BIND_ADDRESS_FAIL,
    //����ʧ��
    BNS_LISTEN_FAIL,
    //��������ʧ��
    BNS_ACCEPT_FAIL,
    //������������Ч��
    BNS_ACCEPT_CLI_IS_NOT_OPEN,
    //�ظ���ʼ��
    BNS_REPEATED_INIT,
    //�ظ��ͷ�
    BNS_REPEATED_RELEASE,
    //����ĵ�ַ�ڵ�
    BNS_ADDRESS_ENDPOINT_ERROR,
    //�յ��׽���
    BNS_EMPTY_SOCKET,
    //δ��֧�ֵĲ���
    BNS_NO_SUPPORT,
    //ͨ��δ����
    BNS_CHANNEL_NOT_CONN,
    //ͨ������ʧ��
    BNS_CHANNEL_CREATE_FAIL,
    //ͨ����ʼ��ʧ��
    BNS_CHANNEL_INIT_FAIL,
    //��Ч�Ĳ�������
    BNS_PARAMS_IS_INVALID,
    //������ֹͣ
    BNS_SERVICE_IS_STOPED,
    //δ֪����
    BNS_NUKNOW_ERROR=-9999,
    
};
//ͨ������
enum class BNS_CHANNEL_TYPE
{
    //δ����
    BNS_INVAILD,
    //tcp �����
    TCP_SERVER,
    //tcp �ͻ���
    TCP_CLIENT,
    //udp
    UDP,
};

//�ڵ�
struct BnsPoint
{
    std::string ip="";
    int port=-1;
};
#ifndef BNS_INVALID_POINT 
#define BNS_INVALID_POINT  BnsPoint()
#endif
//enum class BNS_LOG_LEVEL
//{
//    LERROR,
//    LWARN,
//    LINFO,
//    LDEBUG
//};

//���Ӿ��
typedef std::int64_t BNS_HANDLE;

#define PTR_CAST(x,p) std::reinterpret_pointer_cast<x>(p)
//ת��Ϊ char
#define BNS_EVENT_RECV(buff) std::reinterpret_pointer_cast<char>(buff)

//accept �¼�����
#define BNS_EVENT_ACCEPT(buff) std::reinterpret_pointer_cast<BNS_HANDLE>(buff)

//�¼��ص� ����
//#define BNS_EVENT_DATA(type,buff,buff_len) 

//�¼��ص� buff �����¼������Ͷ���
typedef std::function<void(BNS_HANDLE handle,\
    BNS_NET_EVENT_TYPE type, BNS_ERR_CODE error_code,\
    std::shared_ptr<void> buff,size_t buff_len)> BNS_EVENT_CB;

/*��־����*/
enum BLOG_LEVEL
{
    BL_DEBUG,
    BL_INFO,
    BL_WRAN,
    BL_ERROR,
};
//��־�ص�
typedef std::function<void(BLOG_LEVEL lv,const std::string &log_message)> BNS_LOG_CB;

//�ڴ�����ص�
typedef std::function<std::shared_ptr<char> (size_t memory_size)> BNS_MAKE_SHARED_CB;

//��ʼ��
EXPORT_SYMBOLS BNS_ERR_CODE BNS_Init(int thread_num=0);

//����ʼ��
EXPORT_SYMBOLS BNS_ERR_CODE BNS_DInit();

//����ͨ��
EXPORT_SYMBOLS BNS_ERR_CODE BNS_Add_Channnel(BNS_CHANNEL_TYPE type\
    , const BnsPoint& local,BNS_HANDLE& handle);

//��������
EXPORT_SYMBOLS BNS_ERR_CODE BNS_Connect(BNS_HANDLE handle,const BnsPoint& remote);


//�ر�һ��ͨ��
EXPORT_SYMBOLS BNS_ERR_CODE BNS_Close(BNS_HANDLE handle);

//��������
EXPORT_SYMBOLS BNS_ERR_CODE BNS_Send(BNS_HANDLE handle,\
    std::shared_ptr<char> message, size_t message_size);

//��������
EXPORT_SYMBOLS BNS_ERR_CODE BNS_Send_String(BNS_HANDLE handle,const std::string &message);

//�����¼��ص�
EXPORT_SYMBOLS BNS_ERR_CODE BNS_SetEvntCB(BNS_HANDLE handle, BNS_EVENT_CB cb);

//������־�ص�
EXPORT_SYMBOLS BNS_ERR_CODE BNS_SetLogCB( BNS_LOG_CB cb);

//���û���ص�
EXPORT_SYMBOLS BNS_ERR_CODE BNS_SetMakeSharedCB(BNS_MAKE_SHARED_CB cb);

#endif // !BNS_H_


