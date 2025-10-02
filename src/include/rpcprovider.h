#pragma once
#include "google/protobuf/service.h"
#include "mprpcapplication.h"
#include "muduo/base/Timestamp.h"

#include <memory>
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpConnection.h>
#include <functional>
#include <google/protobuf/descriptor.h>

class RpcProvider
{
public:
    //框架提供给外面使用的用来发布rpc方法的接口
    void NotifyService(google::protobuf::Service *service);

    //启动rpc服务节点，开始提供rpc远程网络调用服务
    void Run();

private:
    //组合EventLoop
    muduo::net::EventLoop m_eventLoop;

    //连接回调
    void OnConnection(const muduo::net::TcpConnectionPtr&);
    //消息回调
    void OnMessage(const muduo::net::TcpConnectionPtr&, muduo::net::Buffer*, muduo::Timestamp);

    struct ServiceInfo
    {
        google::protobuf::Service *m_service; //保存服务对象
        std::unordered_map<std::string,const google::protobuf::MethodDescriptor*> m_methodMap;
    }service_info;

    std::unordered_map<std::string,ServiceInfo> m_serviceInfoMap;

    //Closure的回调操作，用于序列化rpc的响应和网络发送
    void SendRpcResponse(const muduo::net::TcpConnectionPtr&, google::protobuf::Message*);
};