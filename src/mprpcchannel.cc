#include "mprpcapplication.h"
#include "rpcheader.pb.h"
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "logger.h"
#include "zookeeperutil.h"

// 所有通过stub代理对象调用的rpc方法都在这里统一做rpc调用的数据序列化和网络发送
void MprpcChannel::CallMethod(const google::protobuf::MethodDescriptor *method,
                              google::protobuf::RpcController *controller,
                              const google::protobuf::Message *request,
                              google::protobuf::Message *response, 
                              google::protobuf::Closure *done)
{
    const google::protobuf::ServiceDescriptor *sd = method->service();
    std::string service_name = sd->name();
    std::string method_name = method->name();

    //获取参数的序列化长度
    uint32_t args_size = 0;
    std::string args_str;

    if(request->SerializeToString(&args_str))
    {
        args_size = args_str.size();
    }
    else
    {
        controller->SetFailed("serialize request error!");
        return;
    }

    //定义rpc的请求header
    mprpc::RpcHeader rpcHeader;
    rpcHeader.set_service_name(service_name);
    rpcHeader.set_method_name(method_name);
    rpcHeader.set_arg_size(args_size);
    
    uint32_t header_size = 0;
    std::string rpc_header_str;
    if(rpcHeader.SerializeToString(&rpc_header_str))
    {
        header_size = rpc_header_str.size();
    }
    else
    {
        controller->SetFailed("serialize rpc header error!");
        return;
    }

    //组织待发送的rpc请求字符串
    std::string send_rpc_str;
    send_rpc_str.insert(0, std::string((char*)&header_size,4));
    send_rpc_str+=rpc_header_str;
    send_rpc_str+=args_str;

    //打印调试信息
    // std::cout << "=============================" << std::endl;
    // std::cout << "header_size: " << header_size << std::endl;
    // std::cout << "rpc_header_str: " << rpc_header_str << std::endl;
    // std::cout << "service_name: " << service_name << std::endl;
    // std::cout << "method_name: " << method_name << std::endl;
    // std::cout << "args_str: " << args_str << std::endl;
    // std::cout << "=============================" << std::endl;
    LOG_INFO("=========================");
    LOG_INFO("header_size: %d", header_size);
    LOG_INFO("rpc_header_str: %s", rpc_header_str.c_str());
    LOG_INFO("service_name: %s", service_name.c_str());
    LOG_INFO("method_name: %s", method_name.c_str());
    LOG_INFO("args_str: %s", args_str.c_str());
    LOG_INFO("=========================");

    //使用tcp编程，完成rpc方法的远程调用
    int clientfd = socket(AF_INET,SOCK_STREAM,0);
    if(-1 == clientfd)
    {
        char errtext[512] = {0};
        sprintf(errtext, "create socket error! errno:%d",errno);
        controller->SetFailed(errtext);
        return;
    }

    //读取配置文件rpcserver的信息
    // std::string ip = MprpcApplication::getInstance().GetConfig().Load("rpcserverip");
    // uint16_t port = atoi(MprpcApplication::getInstance().GetConfig().Load("rpcserverport").c_str());
    ZkClient zkCli;
    zkCli.Start();
    std::string method_path = "/"+service_name+"/"+method_name;
    std::string host_data = zkCli.GetData(method_path.c_str());
    if(host_data == "")
    {
        controller->SetFailed(method_path+" is not exist!");
        return;
    }
    int idx = host_data.find(":");
    if(idx == -1)
    {
        controller->SetFailed(method_path+" address is invalid!");
        return;
    }
    std::string ip = host_data.substr(0,idx);
    uint16_t port = atoi(host_data.substr(idx+1,host_data.size()-idx).c_str());

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip.c_str());

    //连接rpc服务节点
    if(-1 == connect(clientfd, (struct sockaddr*)&server_addr, sizeof(server_addr)))
    {
        char errtext[512] = {0};
        sprintf(errtext, "connnect error! errno:%d",errno);
        controller->SetFailed(errtext);
        
        close(clientfd);
        return;        
    }

    //发送rpc请求
    if(-1 == send(clientfd, send_rpc_str.c_str(), send_rpc_str.size(), 0))
    {
        char errtext[512] = {0};
        sprintf(errtext, "send error! errno:%d",errno);
        controller->SetFailed(errtext);
        close(clientfd);
        return;
    }

    //接收rpc请求响应值
    char recv_buf[1024] = {0};
    int recv_size = 0;
    if(-1 == (recv_size = recv(clientfd, recv_buf, 1024, 0)))
    {
        char errtext[512] = {0};
        sprintf(errtext, "recv error! errno:%d",errno);
        controller->SetFailed(errtext);
        close(clientfd);
        return;
    }

    //反序列化rpc调用的响应数据
    if(!response->ParseFromArray(recv_buf, recv_size))
    {
        char errtext[512] = {0};
        sprintf(errtext, "parse error! errno:%d",errno);
        controller->SetFailed(errtext);
        close(clientfd);
        return;
    }

    close(clientfd);
}
