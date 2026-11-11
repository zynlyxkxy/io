#ifndef IOGROUP_H
#define IOGROUP_H

#include <QWidget>
#include <QTabWidget>
#include <QGridLayout>
#include <QMap>

#include "io_client.h"

class DiWidget;
class DoWidget;
class AiWidget;
class AoWidget;
class SettingWidget;
class BufferWidget;

class IoGroup : public QWidget
{
    Q_OBJECT

public:
    explicit IoGroup(QWidget *parent = nullptr);
    ~IoGroup();

    void buildTabs(const IoServerInfo &info);
    void setRpcClient(IO_Client_Inherit *rpc);

signals:
    void sig_ObserverReceived(const IoObserverData &data);

private slots:
    void onObserverData(const IoObserverData &data);

private:
    void connectSignals();

    QTabWidget *m_tabWidget;
    IO_Client_Inherit *m_rpc = nullptr;

    QMap<int, AiWidget *> m_aiWidgets;
    QMap<int, AoWidget *> m_aoWidgets;
    QMap<int, DiWidget *> m_diWidgets;
    QMap<int, DoWidget *> m_doWidgets;
    QMap<int, SettingWidget *> m_settingDoubleWidgets;
    QMap<int, SettingWidget *> m_settingBoolWidgets;
    QMap<int, BufferWidget *> m_bufferWidgets;
};

#endif // IOGROUP_H
