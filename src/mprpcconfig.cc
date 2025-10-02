#include "mprpcconfig.h"
#include <iostream>
#include "logger.h"

// 负责解析加载配置文件
void MprpcConfig::LoadConfigFile(const char *config_file)
{
    FILE *pf = fopen(config_file, "r");
    if (nullptr == pf)
    {
        //std::cout << config_file << "is not exist!" << std::endl;
        LOG_ERR("%s is not exist", config_file);
        exit(EXIT_FAILURE);
    }

    // 1.忽略注释   2.提取正确的配置项    3.去掉开头的多余的空格
    while (!feof(pf))
    {
        char buf[512] = {0};
        fgets(buf, 512, pf);

        // 去掉字符串前后多余的空格
        std::string src_buf(buf);
        Trim(src_buf);

        // 判断#的注释
        if (src_buf[0] == '#' || src_buf.empty())
        {
            continue;
        }
        // 解析配置项
        int idx = src_buf.find('=');
        if (idx == std::string::npos)
        {
            // 配置项不合法
            continue;
        }
        std::string key;
        std::string value;        
        
        key = src_buf.substr(0, idx);
        Trim(key);
        value = src_buf.substr(idx + 1, src_buf.size() - idx - 1);
        int endidx = value.find('\n');
        value = value.substr(0,endidx);
        Trim(value);
        m_configMap.insert({key, value});
    }
    fclose(pf);
}

// 查询配置文件信息
std::string MprpcConfig::Load(const std::string &key)
{
    auto it = m_configMap.find(key);
    if (it == m_configMap.end())
    {
        return "";
    }
    return it->second;
}

void MprpcConfig::Trim(std::string &src_buf)
{
    int idx = src_buf.find_first_not_of(' ');
    if (idx != std::string::npos)
    {
        // 说明字符串前面有空格
        src_buf = src_buf.substr(idx, src_buf.size() - idx);
    }

    // 去掉字符串后面多余的空格
    idx = src_buf.find_last_not_of(' ');
    if (idx != std::string::npos)
    {
        // 说明字符串后面有空格
        src_buf = src_buf.substr(0, idx + 1);
    }
}