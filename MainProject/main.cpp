#include <QCoreApplication>
#include <QString>
#include <iostream>
#include "ControlModule/Controller/IoController.h"
#include "ControlModule/IoServer/IoServer.h"
#include "istar_log.h"
#include "istar_platform_env.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    // 配置根：PLATFORM_PATH，未设置时用 cwd（F5 时就是 build\<Config>\）
    const char *envRoot = ISTARGetEnvPlatform();
    const QString root = QString::fromLocal8Bit((envRoot && *envRoot) ? envRoot : ".");
    // 日志根与配置根同一个：<root>/Log/<进程名>/
    ISTARLogInitWithPath(root.toStdString().c_str(), "CTL_MDL_IO");

    std::cout << "[MAIN] Starting IO server... config root=" << root.toStdString() << std::endl;

    // 模块配置：<root>/io_config/config.json（其中 acs/、plc/ 相对该文件所在目录）
    const QString configFile = root + QStringLiteral("/io_config/config.json");

    IoController controller;
    if (!controller.init(configFile))
    {
        std::cout << "[MAIN] IoController init FAILED" << std::endl;
        ISTARLogError("IoController init failed: %s", configFile.toStdString().c_str());
        return -1;
    }
    std::cout << "[MAIN] IoController init OK" << std::endl;

    IoServer server(&controller);
    // mw_config.yaml 是进程级共享文件，固定在配置根上
    const QString mwPath = root + QStringLiteral("/mw_config.yaml");
    std::cout << "[MAIN] IOServerInit mwPath=" << mwPath.toStdString() << std::endl;
    server.IOServerInit(mwPath.toStdString());
    std::cout << "[MAIN] IOServerInit OK" << std::endl;

    server.IOServerRegisterCallback();
    std::cout << "[MAIN] IOServerRegisterCallback OK" << std::endl;

    server.IORun();
    std::cout << "[MAIN] IORun started, entering event loop" << std::endl;

    return app.exec();
}
