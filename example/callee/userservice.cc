#include <iostream>
#include <string>
#include "../user.pb.h"
#include "mprpcapplication.h"
#include "rpcprovider.h"
#include "logger.h"

/*
UserService本来是一个本地服务，提供了两个进程内的本地方法，Login和GetFriendLists
目的：把Login这个本地方法设置成一个RPC的远程方法，不仅可以在这个进程中被调用，还可以在同一台机器的不同进程或者不同机器上的进程被调用
*/

class UserService : public fixbug::UserServiceRpc
{
public:
    bool Login(std::string name, std::string pwd)
    {
        std::cout << "doing local service:Login" << std::endl;
        std::cout << "name: " << name << " pwd: " << pwd << std::endl;
        return true;
    }

    bool Register(uint32_t id, std::string pwd)
    {
        std::cout << "doing local service:Register" << std::endl;
        std::cout << "id: " << id << " pwd: " << pwd << std::endl;
        return true; 
    }

    /*
    这边是callee端，caller发过来的rpc调用请求会交给下面这个Login函数实现
    */
    void Login(::google::protobuf::RpcController *controller,
               const ::fixbug::LoginRequest *request,
               ::fixbug::LoginResponse *response,
               ::google::protobuf::Closure *done)//这个函数是UserService中定义的虚函数，在子类中重写
    {
        //框架给业务上报了请求参数LoginRequest，应用获取相应数据做本地业务
        std::string name = request->name();
        std::string pwd = request->pwd();

        //做本地业务
        bool login_result = Login(name,pwd);

        //把响应写入，包括错误码，错误信息，返回值
        fixbug::ResultCode *code = response->mutable_result();
        code->set_errcode(0);
        code->set_errmsg("");
        response->set_success(login_result);

        //执行回调操作
        done->Run();
    }
    /*
    把一个本地服务变成远程服务需要做两件事情：
    （1）写一个protobuf文件
    （2）在本地业务类中重写生成的user.pb.cc中的UserServiceRpc类中的Login方法
    */

    void Register(::google::protobuf::RpcController *controller,
               const ::fixbug::RegisterResquest *request,
               ::fixbug::RegisterResponse *response,
               ::google::protobuf::Closure *done)
    {
        uint32_t id = request->id();
        std::string pwd = request->pwd();

        bool ret = Register(id,pwd);

        response->mutable_result()->set_errcode(0);
        response->mutable_result()->set_errmsg("");
        response->set_success(ret);

        done->Run();
    }
               
};


int main(int argc, char* argv[])
{
    //调用框架的初始化操作(传入ip地址和端口号)
    MprpcApplication::Init(argc,argv);

    //provider是一个rpc网络服务对象，把UserService对象发布到rpc节点上,这样别人也可以使用这个Login方法了
    RpcProvider provider;
    provider.NotifyService(new UserService());//堆区开辟的，是不是应该考虑释放
    //启动一个rpc服务节点发布 Run之后 进程进入阻塞状态，等待远程的rpc调用请求，即等着别人来用我这个Login方法
    provider.Run();

    return 0;

}