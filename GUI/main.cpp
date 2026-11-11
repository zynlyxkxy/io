#include <QApplication>
#include <QFont>
#include "MainWindow/MainWindow.h"
#include "io_client.h"

static void registerMetaTypes()
{
    qRegisterMetaType<IoObserverData>("IoObserverData");
}

int main(int argc, char *argv[])
{
    registerMetaTypes();

    QApplication app(argc, argv);

    QFont defaultFont("Microsoft YaHei", 10);
    defaultFont.setStyleHint(QFont::SansSerif);
    app.setFont(defaultFont);

    MainWindow w;
    w.show();

    return app.exec();
}
