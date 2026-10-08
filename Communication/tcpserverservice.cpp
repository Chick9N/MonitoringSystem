#include "tcpserverservice.h"
#include "mockmodbustcpserver.h"

namespace {

class MockTcpServerService final : public TcpServerService
{
public:
    explicit MockTcpServerService(QObject *parent)
        : TcpServerService(parent)
        , m_server(new MockModbusTCPServer(this))
    {
    }

    bool start(quint16 port) override
    {
        if (!m_server->start(port)) {
            emit errorOccurred(m_server->errorString());
            return false;
        }
        emit stateChanged(true, QStringLiteral("TCP服务器运行中，端口：%1").arg(port));
        return true;
    }

    void stop() override
    {
        if (!m_server->isRunning())
            return;
        m_server->stop();
        emit stateChanged(false, QStringLiteral("TCP服务器已关闭"));
    }

    bool isRunning() const override
    {
        return m_server->isRunning();
    }

private:
    MockModbusTCPServer *m_server;
};

} // namespace

TcpServerService *createTcpServerService(QObject *parent)
{
    return new MockTcpServerService(parent);
}
