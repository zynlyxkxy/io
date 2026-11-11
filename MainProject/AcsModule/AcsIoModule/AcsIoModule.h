#ifndef ACSIOMODULE_H
#define ACSIOMODULE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QHash>
#include <QVariant>

class AcsClient;
class QTimer;

class AcsIoModule : public QObject
{
    Q_OBJECT
public:
    enum Error {
        Success = 0,
        DriverNotConnected,
    };

    explicit AcsIoModule(const QString &ip, int port, QObject *parent = nullptr);
    ~AcsIoModule();

    // --- AO：写模拟量输出 ---
    int setAo(const QString &varName, double value);

    // --- DO：写数字量输出 ---
    int setDo(const QString &varName, bool value);

    // --- AI：读模拟量输入 ---
    int getAi(const QString &varName, double &out, bool forceRefresh = false);

    // --- DI：读数字量输入 ---
    int getDi(const QString &varName, bool &out, bool forceRefresh = false);

    // --- Buffer ---
    int startBuffer(int bufferId);
    int stopBuffer(int bufferId);
    int getBufferRunning(int bufferId, bool &out, bool forceRefresh = false);

    // --- 注册观察量到轮询 ---
    void registerAi(const QString &varName);
    void registerDi(const QString &varName);
    void registerBuffer(int bufferId);

    // --- 轮询 ---
    void setPollPeriod(int ms);

signals:
    void sig_ObsRefreshed();

private:
    void onPollTimer();

    AcsClient *m_client;
    QTimer *m_pollTimer;

    QHash<QString, QVariant> m_cache;

    QStringList m_registeredAi;
    QStringList m_registeredDi;
    QList<int> m_registeredBuffers;
};

#endif // ACSIOMODULE_H
