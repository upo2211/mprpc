set(SRC_LIST userservice.cc ../user.pb.cc)
# rpc方法的提供者和使用者都是生成可执行文件
add_executable(provider ${SRC_LIST})
target_link_libraries(provider mprpc protobuf)