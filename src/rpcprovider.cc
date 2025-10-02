#include "rpcprovider.h"
#include "rpcheader.pb.h"
#include "logger.h"
#include "zookeeperutil.h"

// 框架提供给外面使用的用来发布rpc方法的接口
void RpcProvider::NotifyService(google::protobuf::Service *service)
{
    // 获取了服务对象的描述信息
    const google::protobuf::ServiceDescriptor *pserverDesc = service->GetDescriptor();

    // 获取服务的名字
    std::string service_name = pserverDesc->name();
    // 获取服务对象service的方法的数量
    int methodCnt = pserverDesc->method_count();
    LOG_INFO("service_name: %s", service_name.c_str());
    // 获取对应的方法
    for (int i = 0; i < methodCnt; ++i)
    {
        const google::protobuf::MethodDescriptor *pmethosDesc = pserverDesc->method(i);
        std::string method_name = pmethosDesc->name();
        service_info.m_methodMap.insert({method_name, pmethosDesc});
        LOG_INFO("method_name: %s", method_name.c_str());
    }
    service_info.m_service = service;
    m_serviceInfoMap.insert({service_name, service_info});
}

// 启动rpc服务节点，开始提供rpc远程网络调用服务
void RpcProvider::Run()
{
    // 配置ip和port
    std::string ip = MprpcApplication::getInstance().GetConfig().Load("rpcserverip");
    uint16_t port = atoi(MprpcApplication::getInstance().GetConfig().Load("rpcserverport").c_str());

    muduo::net::InetAddress address(ip, port);

    // 启动tcpserver
    muduo::net::TcpServer server(&m_eventLoop, address, "RpcProvider");
    // 绑定连接回调和消息读写回调方法
    server.setConnectionCallback(std::bind(&RpcProvider::OnConnection, this,
                                           std::placeholders::_1));
    server.setMessageCallback(std::bind(&RpcProvider::OnMessage, this, std::placeholders::_1,
                                        std::placeholders::_2,
                                        std::placeholders::_3));
    // 设置muduo网络库的线程数量
    server.setThreadNum(4); // 一个是IO线程，其他都是工作线程

    //把当前rpc节点上要发布的服务全部注册到zk上面，让rpc client可以从zk上发现服务
    //session timeout 30s   zkclient 网络I/O线程  1/3*timeout 时间发送ping消息
    ZkClient zkCli;
    zkCli.Start();
    //service_name为永久性节点，method_name为临时性节点
    for(auto &sp:m_serviceInfoMap)
    {
        std::string service_path = "/"+sp.first;
        zkCli.Create(service_path.c_str(),nullptr,0);
        for(auto &mp:sp.second.m_methodMap)
        {
            // /service_name/method_name   /UserServiceRpc/Login存储当前这个rpc服务节点主机的ip和port
            std::string method_path = service_path+"/"+mp.first;
            char method_path_data[128] = {0};
            sprintf(method_path_data,"%s:%d",ip.c_str(),port);
            zkCli.Create(method_path.c_str(),method_path_data,strlen(method_path_data),ZOO_EPHEMERAL);
        }
    }

    // 启动网络服务
    std::cout << "RpcProvider start service at ip: " << ip << " port: " << port << std::endl;

    // 启动网络服务
    server.start();
    m_eventLoop.loop();
}

void RpcProvider::OnConnection(const muduo::net::TcpConnectionPtr &conn)
{
    if (!conn->connected())
    {
        conn->shutdown();
    }
}

void RpcProvider::OnMessage(const muduo::net::TcpConnectionPtr &conn,
                            muduo::net::Buffer *buffer,
                            muduo::Timestamp time)
{
    // 网络上接收的远程rpc调用请求的字符流
    std::string recv_buf = buffer->retrieveAllAsString();

    // 从字符流中读取前4个字节的内容
    uint32_t header_size = 0;
    // recv_buf从第0个字节开始，拷贝4个字节放到header_size这个内存地址上
    recv_buf.copy((char *)&header_size, 4, 0);

    std::string rpc_header_str = recv_buf.substr(4, header_size);
    mprpc::RpcHeader rpcHeader;
    std::string service_name;
    std::string method_name;
    uint32_t args_size;
    if (rpcHeader.ParseFromString(rpc_header_str))
    {
        // 数据头反序列化成功
        service_name = rpcHeader.service_name();
        method_name = rpcHeader.method_name();
        args_size = rpcHeader.arg_size();
    }
    else
    {
        // 数据头反序列化失败
        //std::cout << "rpc_header_str:" << rpc_header_str << " parse error" << std::endl;
        LOG_ERR("rpc_header_str: %s", rpc_header_str.c_str());
        return;
    }

    // 获取rpc方法参数的字符流数据
    std::string args_str = recv_buf.substr(4 + header_size, args_size); // 防止粘包问题

    // 打印调试信息
    std::cout << "======================" << std::endl;
    std::cout << "header_size: " << header_size << std::endl;
    std::cout << "rpc_header_str: " << rpc_header_str << std::endl;
    std::cout << "service_name: " << service_name << std::endl;
    std::cout << "method_name: " << method_name << std::endl;
    std::cout << "args_str: " << args_str << std::endl;
    std::cout << "======================" << std::endl;

    // 获取service对象和method对象
    auto it = m_serviceInfoMap.find(service_name);
    if (it == m_serviceInfoMap.end())
    {
        std::cout << service_name << " is not exist!" << std::endl;
        return;
    }

    auto mit = it->second.m_methodMap.find(method_name);
    if (mit == it->second.m_methodMap.end())
    {
        std::cout << service_name << ":" << method_name << " is not exist!" << std::endl;
        return;
    }

    google::protobuf::Service *service = it->second.m_service;
    const google::protobuf::MethodDescriptor *method = mit->second;

    // 生成rpc方法调用的请求request和响应response参数
    google::protobuf::Message *request = service->GetRequestPrototype(method).New();
    if (!request->ParseFromString(args_str))
    {
        std::cout << "request parse error, content: " << args_str << std::endl;
        return;
    }

    google::protobuf::Message *response = service->GetResponsePrototype(method).New();

    // 给下面的method方法的调用绑定一个Closure的回调函数
    google::protobuf::Closure *done = google::protobuf::NewCallback<RpcProvider,
                                                                    const muduo::net::TcpConnectionPtr &,
                                                                    google::protobuf::Message *>
                                                                    (this,
                                                                    &RpcProvider::SendRpcResponse,
                                                                    conn, response);

    // 在框架上根据远端rpc请求，调用当前rpc节点上发布的方法
    service->CallMethod(method, nullptr, request, response, done);
}

// Closure的回调操作，用于序列化rpc的响应和进行网络发送
void RpcProvider::SendRpcResponse(const muduo::net::TcpConnectionPtr &conn, google::protobuf::Message *response)
{
    std::string response_str;
    if(response->SerializeToString(&response_str))
    {
        //序列化成功后，通过网络把rpc方法执行的结果发送回rpc的调用方
        conn->send(response_str);
    }
    else
    {
        std::cout << "serialize response_str error!" << std::endl;
    }
    conn->shutdown();//模拟http的短链接服务，由rpcprovider主动断开连接
}