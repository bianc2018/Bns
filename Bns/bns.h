/*
    基于boost.asio 封装的基础网络库
    hql 2020/01/04
*/
#ifndef BNS_H_
#define BNS_H_

#include <memory>
#include <functional>
#include <string>

//定义导出符号
#ifdef _WIN32
#define EXPORT_SYMBOLS extern "C" __declspec(dllexport)
#else
#define EXPORT_SYMBOLS 
#endif

//网络事件回调类型
enum class BNS_NET_EVENT_TYPE
{
    //链接确立 establish 
    BNS_CONNECT_ESTABLISH = 1,
    //接收到链接
    BNS_ACCEPT ,
    //接收到数据
    BNS_RECV_DATA ,
    //发送数据完整 complete
    BNS_SEND_COMPLETE,
    //链接已经关闭
    BNS_CLOSED,
};
//错误码
enum class BNS_ERR_CODE
{
    //无错误
    BNS_OK,
    //内存申请失败
    BNS_ALLOC_FAIL = -998,
    //通道类型不对
    BNS_CHANNEL_TYPE_FAIL,
    //无效的句柄
    BNS_INVAILD_HANDLE,
    //重复的句柄
    BNS_HANDLE_IS_USED,
    //创建套接字失败
    BNS_CREATE_SOCKET_FAIL,
    //链接失败
    BNS_CONNECT_FAIL,
    //接收数据失败
    BNS_RECV_DATA_FAIL,
    //发送数据失败
    BNS_SEND_DATA_FAIL,
    //空的事件回调
    BNS_EMPYTY_EVENT_CB,
    //空的日志回调
    BNS_EMPYTY_LOG_CB,
    //空的内存申请回调
    BNS_EMPYTY_MAKE_SHARED_CB,
    //空的接收函数
    BNS_EMPYTY_ACCEPT_CB,
    //空的BUFF
    BNS_EMPYTY_BUFF,
    //打开失败
    BNS_SOCKET_OPEN_FAIL,
    //重用地址失败
    BNS_SET_REUSE_ADDRESS_FAIL,
    //绑定地址失败
    BNS_BIND_ADDRESS_FAIL,
    //监听失败
    BNS_LISTEN_FAIL,
    //接收链接失败
    BNS_ACCEPT_FAIL,
    //接收链接是无效的
    BNS_ACCEPT_CLI_IS_NOT_OPEN,
    //重复初始化
    BNS_REPEATED_INIT,
    //重复释放
    BNS_REPEATED_RELEASE,
    //错误的地址节点
    BNS_ADDRESS_ENDPOINT_ERROR,
    //空的套接字
    BNS_EMPTY_SOCKET,
    //未受支持的操作
    BNS_NO_SUPPORT,
    //通道未链接
    BNS_CHANNEL_NOT_CONN,
    //通道创建失败
    BNS_CHANNEL_CREATE_FAIL,
    //通道初始化失败
    BNS_CHANNEL_INIT_FAIL,
    //无效的参数输入
    BNS_PARAMS_IS_INVALID,
    //服务已停止
    BNS_SERVICE_IS_STOPED,
    //未知错误
    BNS_NUKNOW_ERROR=-9999,
    
};
//通道类型
enum class BNS_CHANNEL_TYPE
{
    //未定义
    BNS_INVAILD,
    //tcp 服务端
    TCP_SERVER,
    //tcp 客户端
    TCP_CLIENT,
    //udp
    UDP,
};

//节点
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

//链接句柄
typedef std::int64_t BNS_HANDLE;

#define PTR_CAST(x,p) std::reinterpret_pointer_cast<x>(p)
//转换为 char
#define BNS_EVENT_RECV(buff) std::reinterpret_pointer_cast<char>(buff)

//accept 事件数据
#define BNS_EVENT_ACCEPT(buff) std::reinterpret_pointer_cast<BNS_HANDLE>(buff)

//事件回调 数据
//#define BNS_EVENT_DATA(type,buff,buff_len) 

//事件回调 buff 根据事件的类型而变
typedef std::function<void(BNS_HANDLE handle,\
    BNS_NET_EVENT_TYPE type, BNS_ERR_CODE error_code,\
    std::shared_ptr<void> buff,size_t buff_len)> BNS_EVENT_CB;

/*日志级别*/
enum BLOG_LEVEL
{
    BL_DEBUG,
    BL_INFO,
    BL_WRAN,
    BL_ERROR,
};
//日志回调
typedef std::function<void(BLOG_LEVEL lv,const std::string &log_message)> BNS_LOG_CB;

//内存申请回调
typedef std::function<std::shared_ptr<char> (size_t memory_size)> BNS_MAKE_SHARED_CB;

//初始化
EXPORT_SYMBOLS BNS_ERR_CODE BNS_Init(int thread_num=0);

//反初始化
EXPORT_SYMBOLS BNS_ERR_CODE BNS_DInit();

//增加通道
EXPORT_SYMBOLS BNS_ERR_CODE BNS_Add_Channnel(BNS_CHANNEL_TYPE type\
    , const BnsPoint& local,BNS_HANDLE& handle);

//建立链接
EXPORT_SYMBOLS BNS_ERR_CODE BNS_Connect(BNS_HANDLE handle,const BnsPoint& remote);


//关闭一个通道
EXPORT_SYMBOLS BNS_ERR_CODE BNS_Close(BNS_HANDLE handle);

//发送数据
EXPORT_SYMBOLS BNS_ERR_CODE BNS_Send(BNS_HANDLE handle,\
    std::shared_ptr<char> message, size_t message_size);

//发送数据
EXPORT_SYMBOLS BNS_ERR_CODE BNS_Send_String(BNS_HANDLE handle,const std::string &message);

//设置事件回调
EXPORT_SYMBOLS BNS_ERR_CODE BNS_SetEvntCB(BNS_HANDLE handle, BNS_EVENT_CB cb);

//设置日志回调
EXPORT_SYMBOLS BNS_ERR_CODE BNS_SetLogCB( BNS_LOG_CB cb);

//设置缓存回调
EXPORT_SYMBOLS BNS_ERR_CODE BNS_SetMakeSharedCB(BNS_MAKE_SHARED_CB cb);

#endif // !BNS_H_


