#ifndef SYSTEMTESTDIALOG_H
#define SYSTEMTESTDIALOG_H

#include <QDialog>
#include <QVector>

class QLabel;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;
class MockModbusTCPServer;
class ModbusTCP;

class SystemTestDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SystemTestDialog(QWidget *parent = nullptr);

signals:
    void testsFinished(int passed, int failed);

private slots:
    void runAllTests();
    void handleTcpConnected();
    void handleTcpDisconnected();
    void handleTcpResponse(const QByteArray &response);

private:
    enum class TcpPhase { Idle, InitialRead, WaitingDisconnect, WaitingReconnect, RestartRead, ExceptionRead, Done };
    void runAlarmScenario();
    void runSimulationRangeScenario();
    void runDatabaseSnapshotScenario();
    void startTcpScenario();
    void sendTcpRead(quint8 unitId, quint16 transactionId);
    void pass(const QString &name, const QString &detail = {});
    void fail(const QString &name, const QString &detail);
    void finish();

    QLabel *m_summaryLabel = nullptr;
    QProgressBar *m_progressBar = nullptr;
    QPlainTextEdit *m_logView = nullptr;
    QPushButton *m_runButton = nullptr;
    QPushButton *m_copyButton = nullptr;
    MockModbusTCPServer *m_server = nullptr;
    ModbusTCP *m_client = nullptr;
    TcpPhase m_tcpPhase = TcpPhase::Idle;
    quint16 m_testPort = 0;
    int m_passCount = 0;
    int m_failCount = 0;
    int m_completedCount = 0;
    int m_tcpErrorCount = 0;
    bool m_finishing = false;
};

#endif // SYSTEMTESTDIALOG_H
