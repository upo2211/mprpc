aux_source_directory(. SRC_LIST)
# 使用框架的代码生成一个动态库
add_library(mprpc ${SRC_LIST})
# 设置依赖项
target_link_libraries(mprpc muduo_net muduo_base pthread zookeeper_mt)