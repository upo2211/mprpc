#pragma once

#include <unordered_map>
#include <string>
#include <stdio.h>

//读取配置文件的类
class MprpcConfig
{
public:
    //负责解析加载配置文件
    void LoadConfigFile(const char*config_file);

    //查询配置文件信息
    std::string Load(const std::string &key);

    //去掉字符串前后的空格
    void Trim(std::string &str);

private:
    std::unordered_map<std::string, std::string> m_configMap;
};