#ifndef TCPSERVERSERVICE_H
#define TCPSERVERSERVICE_H

#include <QObject>
#include <QString>
#include <QtGlobal>

class TcpServerService : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;
    ~TcpServerService() override = default;

    virtual bool start(quint16 port) = 0;
    virtual void stop() = 0;
    virtual bool isRunning() const = 0;

signals:
    void stateChanged(bool running, const QString &message);
    void errorOccurred(const QString &message);
};

TcpServerService *createTcpServerService(QObject *parent = nullptr);

#endif // TCPSERVERSERVICE_H
