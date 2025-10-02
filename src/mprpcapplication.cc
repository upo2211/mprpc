#include "mprpcapplication.h"
#include <iostream>
#include <unistd.h>
#include "logger.h"

MprpcConfig MprpcApplication::m_config = MprpcConfig();

void showArgHelp()
{
    std::cout << "format:command -i <configure>" << std::endl;
}

void MprpcApplication::Init(int argc, char *argv[])
{
    if (argc < 2)
    {
        showArgHelp();
        exit(EXIT_FAILURE);
    }

    int c = 0;
    std::string config_file;
    while ((c = getopt(argc, argv, "i:")) != -1)
    {
        switch(c)
        {
        case 'i':
            config_file = optarg;
            break;
        case '?':
            //std::cout << "incalid args!" << std::endl;
            LOG_ERR("incalid args!");
            showArgHelp();
            exit(EXIT_FAILURE);
        case ':'://在某些实现中，getpot选项缺少参数的时候可能会返回'?'而不是'?'
            //std::cout << "need <configure>" << std::endl;
            LOG_ERR("need <configure>");
            showArgHelp();
            exit(EXIT_FAILURE);
        default:
            break;
        }
    }

    m_config.LoadConfigFile(config_file.c_str());

}

MprpcApplication &MprpcApplication::getInstance()
{
    static MprpcApplication app;
    return app;
}

MprpcConfig& MprpcApplication::GetConfig()
{
    return m_config;
}

