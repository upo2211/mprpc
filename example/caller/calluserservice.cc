#include "mprpcapplication.h"
#include <iostream>
#include "user.pb.h"
#include "mprpcchannel.h"
#include "logger.h"

int main(int argc, char *argv[])
{
    // 整个程序启动之后，想使用mprpc框架来使用rpc服务，需要先调用框架的初始化函数
    MprpcApplication::Init(argc, argv);

    // 演示调用远程发布的rpc方法Login
    fixbug::UserServiceRpc_Stub stub(new MprpcChannel());

    fixbug::LoginRequest request;
    request.set_name("zhangsan");
    request.set_pwd("123456");
    fixbug::LoginResponse response;

    MprpcController controller;
    stub.Login(&controller, &request, &response, nullptr);

    // 一次rpc调用完成，读取调用的结果
    if (controller.Failed())
    {
        std::cout << controller.ErrorText() << std::endl;
    }
    else
    {
        if (0 == response.result().errcode())
        {
            std::cout << "rpc login response success: " << response.success() << std::endl;
        }
        else
        {
            std::cout << "rpc login response error: " << response.result().errmsg() << std::endl;
        }
    }
    return 0;
}