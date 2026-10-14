#include "systemtestdialog.h"

#include "../Communication/mockmodbustcpserver.h"
#include "../Communication/modbustcp.h"
#include "../Core/alarmmanager.h"
#include "../Core/device.h"
#include "../Data/databasemanager.h"

#include <QDateTime>
#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTimer>
#include <QVBoxLayout>
#include <cstdio>

SystemTestDialog::SystemTestDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("系统一键自检"));
    setMinimumSize(680, 480);
    setWindowIcon(QIcon(QStringLiteral(":/assets/monitoring-app.svg")));

    auto *layout = new QVBoxLayout(this);
    auto *intro = new QLabel(
        QStringLiteral("运行隔离式演示自检：覆盖报警/恢复、模拟越限、Modbus TCP读数、异常响应和服务器重启重连。现有设备与数据库不会被修改。"),
        this);
    intro->setWordWrap(true);
    layout->addWidget(intro);

    m_summaryLabel = new QLabel(QStringLiteral("准备就绪"), this);
    layout->addWidget(m_summaryLabel);
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 9);
    m_progressBar->setValue(0);
    layout->addWidget(m_progressBar);

    m_logView = new QPlainTextEdit(this);
    m_logView->setReadOnly(true);
    m_logView->setMaximumBlockCount(500);
    layout->addWidget(m_logView, 1);

    auto *buttons = new QHBoxLayout;
    buttons->addStretch();
    m_runButton = new QPushButton(QStringLiteral("运行一键自检"), this);
    m_runButton->setMinimumWidth(150);
    buttons->addWidget(m_runButton);
    auto *closeButton = new QPushButton(QStringLiteral("关闭"), this);
    buttons->addWidget(closeButton);
    m_copyButton = new QPushButton(QStringLiteral("复制报告"), this);
    buttons->insertWidget(1, m_copyButton);
    layout->addLayout(buttons);

    connect(m_runButton, &QPushButton::clicked, this, &SystemTestDialog::runAllTests);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::close);
    connect(m_copyButton, &QPushButton::clicked, this, [this]() {
        QApplication::clipboard()->setText(m_summaryLabel->text() + "\n" + m_logView->toPlainText());
    });
}

void SystemTestDialog::runAllTests()
{
    if (m_tcpPhase != TcpPhase::Idle && m_tcpPhase != TcpPhase::Done)
        return;

    if (m_client) {
        delete m_client;
        m_client = nullptr;
    }
    if (m_server) {
        m_server->stop();
        delete m_server;
        m_server = nullptr;
    }

    m_passCount = 0;
    m_failCount = 0;
    m_completedCount = 0;
    m_tcpErrorCount = 0;
    m_finishing = false;
    m_tcpPhase = TcpPhase::Idle;
    m_logView->clear();
    m_progressBar->setValue(0);
    m_summaryLabel->setText(QStringLiteral("自检进行中…"));
    m_runButton->setEnabled(false);

    runAlarmScenario();
    runSimulationRangeScenario();
    runDatabaseSnapshotScenario();
    startTcpScenario();
}

void SystemTestDialog::runAlarmScenario()
{
    AlarmManager alarmManager;
    QList<AlarmInfo> events;
    connect(&alarmManager, &AlarmManager::alarmTriggered, this,
            [&events](const AlarmInfo &alarm) { events.append(alarm); });

    DeviceConfig config;
    config.temperatureLow = 15.0;
    config.temperatureHigh = 50.0;
    config.voltageLow = 210.0;
    config.voltageHigh = 230.0;
    const int testDeviceId = 900001;

    DeviceData data;
    data.temperature = 25.0;
    data.voltage = 220.0;
    data.isOnline = true;
    alarmManager.checkDeviceData(testDeviceId, data, config);

    auto testAlarmAndRecovery = [&](AlarmType type, double temperature, double voltage) {
        data.temperature = temperature;
        data.voltage = voltage;
        alarmManager.checkDeviceData(testDeviceId, data, config);
        const int starts = events.size();
        alarmManager.checkDeviceData(testDeviceId, data, config); // 重复越限不应重复报警
        const bool deduplicated = events.size() == starts;
        data.temperature = 25.0;
        data.voltage = 220.0;
        alarmManager.checkDeviceData(testDeviceId, data, config);
        return deduplicated && events.size() == starts + 1 &&
               events.last().type == type && events.last().recovered;
    };

    const bool highOk = testAlarmAndRecovery(AlarmType::TemperatureHigh, 55.0, 220.0);
    const bool lowOk = testAlarmAndRecovery(AlarmType::TemperatureLow, 10.0, 220.0);
    const bool voltageHighOk = testAlarmAndRecovery(AlarmType::VoltageHigh, 25.0, 240.0);
    const bool voltageLowOk = testAlarmAndRecovery(AlarmType::VoltageLow, 25.0, 200.0);

    data.isOnline = false;
    alarmManager.checkDeviceData(testDeviceId, data, config);
    const bool offlineAlarm = !events.isEmpty() && events.last().type == AlarmType::DeviceOffline && !events.last().recovered;
    data.isOnline = true;
    alarmManager.checkDeviceData(testDeviceId, data, config);
    const bool offlineRecovery = events.size() >= 2 && events.last().type == AlarmType::DeviceOffline && events.last().recovered;

    const bool passed = highOk && lowOk && voltageHighOk && voltageLowOk && offlineAlarm && offlineRecovery && events.size() == 10;
    if (passed)
        pass(QStringLiteral("报警触发、去重与恢复"), QStringLiteral("温度/电压上下限及离线恢复均通过，共 %1 条报警事件").arg(events.size()));
    else
        fail(QStringLiteral("报警触发、去重与恢复"), QStringLiteral("预期10条报警事件，实际%1条").arg(events.size()));
}

void SystemTestDialog::runSimulationRangeScenario()
{
    Device device(-900001);
    device.start();
    double minTemperature = 1000.0, maxTemperature = -1000.0;
    double minVoltage = 1000.0, maxVoltage = -1000.0;
    for (int i = 0; i < 90; ++i) {
        device.updateData();
        minTemperature = qMin(minTemperature, device.data().temperature);
        maxTemperature = qMax(maxTemperature, device.data().temperature);
        minVoltage = qMin(minVoltage, device.data().voltage);
        maxVoltage = qMax(maxVoltage, device.data().voltage);
    }

    const bool passed = minTemperature < 15.0 && maxTemperature > 50.0 &&
                        minVoltage < 210.0 && maxVoltage > 230.0;
    if (passed)
        pass(QStringLiteral("模拟数据越限覆盖"),
             QStringLiteral("温度 %1～%2℃，电压 %3～%4V")
                 .arg(minTemperature, 0, 'f', 1).arg(maxTemperature, 0, 'f', 1)
                 .arg(minVoltage, 0, 'f', 1).arg(maxVoltage, 0, 'f', 1));
    else
        fail(QStringLiteral("模拟数据越限覆盖"), QStringLiteral("90个采样点未覆盖报警上下限"));
}

void SystemTestDialog::runDatabaseSnapshotScenario()
{
    // 使用当前工作目录下唯一的临时文件，避免依赖系统临时目录权限。
    const QString databasePath = QDir::current().filePath(
        QStringLiteral("monitoring_self_test_%1_%2.db")
            .arg(QCoreApplication::applicationPid())
            .arg(QDateTime::currentMSecsSinceEpoch()));
    QFile::remove(databasePath);
    const QString connectionName = QStringLiteral("self_test_seed_connection");
    QString setupError;
    {
        QSqlDatabase database = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        database.setDatabaseName(databasePath);
        if (!database.open()) {
            setupError = database.lastError().text();
        } else {
            {
                QSqlQuery query(database);
                const QStringList schema = {
                    QStringLiteral("CREATE TABLE device (device_id INTEGER PRIMARY KEY, device_name TEXT, data_source INTEGER, protocol_type INTEGER, serial_port TEXT, baud_rate INTEGER, data_bits INTEGER, stop_bits INTEGER, parity TEXT, rtu_slave_id INTEGER, rtu_start_address INTEGER, rtu_quantity INTEGER, tcp_ip TEXT, tcp_port INTEGER, tcp_unit_id INTEGER, tcp_start_address INTEGER, tcp_quantity INTEGER, temperature_low REAL, temperature_high REAL, voltage_low REAL, voltage_high REAL)"),
                    QStringLiteral("CREATE TABLE alarm_history (id INTEGER PRIMARY KEY, device_id INTEGER, alarm_type INTEGER, message TEXT, recovered INTEGER, timestamp TEXT, acknowledged INTEGER)"),
                    QStringLiteral("CREATE TABLE device_history (id INTEGER PRIMARY KEY, device_id INTEGER, temperature REAL, voltage REAL, online INTEGER, timestamp TEXT)")
                };
                for (const QString &statement : schema) {
                    if (!query.exec(statement)) {
                        setupError = query.lastError().text();
                        break;
                    }
                }

                if (setupError.isEmpty()) {
                    const QStringList rows = {
                        QStringLiteral("INSERT INTO device VALUES (41, '自检存储设备', 2, 2, '', 9600, 8, 1, 'None', 1, 0, 3, '127.0.0.1', 1502, 1, 0, 3, 12.5, 48.5, 205.0, 235.0)"),
                        QStringLiteral("INSERT INTO alarm_history VALUES (1, 41, 0, '温度过高', 0, '2026-10-10 12:00:00', 1)"),
                        QStringLiteral("INSERT INTO alarm_history VALUES (2, 41, 4, '设备恢复', 1, '2026-10-10 12:01:00', 1)"),
                        QStringLiteral("INSERT INTO device_history VALUES (1, 41, 51.2, 220.0, 1, '2026-10-10 12:00:00')"),
                        QStringLiteral("INSERT INTO device_history VALUES (2, 41, 49.8, 221.0, 1, '2026-10-10 12:01:00')")
                    };
                    for (const QString &statement : rows) {
                        if (!query.exec(statement)) {
                            setupError = query.lastError().text();
                            break;
                        }
                    }
                }
            }
            database.close();
        }
        database = QSqlDatabase();
    }
    QSqlDatabase::removeDatabase(connectionName);

    if (!setupError.isEmpty()) {
        QFile::remove(databasePath);
        fail(QStringLiteral("SQLite 快照读取"),
             QStringLiteral("%1（测试数据库：%2）").arg(setupError, databasePath));
        return;
    }

    const DatabaseSnapshot snapshot = DatabaseManager::loadStartupSnapshot(databasePath);
    const QList<DeviceHistory> history = DatabaseManager::loadDeviceHistorySnapshot(
        databasePath,
        41,
        QDateTime::fromString(QStringLiteral("2026-10-10 11:59:00"), "yyyy-MM-dd HH:mm:ss"),
        QDateTime::fromString(QStringLiteral("2026-10-10 12:02:00"), "yyyy-MM-dd HH:mm:ss"));
    QFile::remove(databasePath);

    const bool passed = snapshot.devices.size() == 1 &&
        snapshot.devices.first().deviceId == 41 &&
        snapshot.devices.first().deviceName == QStringLiteral("自检存储设备") &&
        qAbs(snapshot.devices.first().temperatureLow - 12.5) < 0.001 &&
        snapshot.alarmHistory.size() == 2 &&
        snapshot.activeAlarms.size() == 1 &&
        snapshot.activeAlarms.first().type == AlarmType::TemperatureHigh &&
        snapshot.activeAlarms.first().acknowledged &&
        history.size() == 2 &&
        qAbs(history.first().data.temperature - 51.2) < 0.001;

    if (passed) {
        pass(QStringLiteral("SQLite 快照读取"),
             QStringLiteral("设备阈值、报警历史、活动报警和历史采样均从临时数据库正确读回"));
    } else {
        fail(QStringLiteral("SQLite 快照读取"),
             QStringLiteral("设备数量 %1，报警历史 %2，活动报警 %3，历史采样 %4")
                 .arg(snapshot.devices.size())
                 .arg(snapshot.alarmHistory.size())
                 .arg(snapshot.activeAlarms.size())
                 .arg(history.size()));
    }
}

void SystemTestDialog::startTcpScenario()
{
    m_server = new MockModbusTCPServer(this);
    if (!m_server->start(0)) {
        fail(QStringLiteral("Modbus TCP本地测试服务"), m_server->errorString());
        finish();
        return;
    }
    m_testPort = m_server->serverPort();
    if (m_testPort == 0) {
        fail(QStringLiteral("Modbus TCP本地测试服务"), QStringLiteral("系统未分配本地端口"));
        finish();
        return;
    }
    pass(QStringLiteral("Modbus TCP本地测试服务"), QStringLiteral("已监听 127.0.0.1:%1").arg(m_testPort));

    m_client = new ModbusTCP(this);
    connect(m_client, &ModbusTCP::tcpConnected, this, &SystemTestDialog::handleTcpConnected);
    connect(m_client, &ModbusTCP::tcpDisconnected, this, &SystemTestDialog::handleTcpDisconnected);
    connect(m_client, &ModbusTCP::dataReceived, this, &SystemTestDialog::handleTcpResponse);
    connect(m_client, &ModbusTCP::tcpError, this,
            [this](const QString &) { ++m_tcpErrorCount; });

    QTimer::singleShot(12000, m_client, [this]() {
        if (m_tcpPhase != TcpPhase::Done) {
            fail(QStringLiteral("Modbus TCP通信/重连"), QStringLiteral("12秒内未完成读数与重连验证"));
            if (m_server)
                m_server->stop();
            finish();
        }
    });

    m_tcpPhase = TcpPhase::InitialRead;
    m_client->connectToDevice(QStringLiteral("127.0.0.1"), m_testPort);
}

void SystemTestDialog::handleTcpConnected()
{
    if (m_tcpPhase == TcpPhase::InitialRead)
        sendTcpRead(1, 1);
    else if (m_tcpPhase == TcpPhase::WaitingReconnect) {
        pass(QStringLiteral("TCP服务器重启后自动重连"));
        m_tcpPhase = TcpPhase::RestartRead;
        sendTcpRead(1, 2);
    }
}

void SystemTestDialog::handleTcpDisconnected()
{
    if (m_tcpPhase != TcpPhase::WaitingDisconnect)
        return;

    m_tcpPhase = TcpPhase::WaitingReconnect;
    QTimer::singleShot(150, this, [this]() {
        if (!m_server || !m_server->start(m_testPort)) {
            fail(QStringLiteral("TCP服务器重启"), m_server ? m_server->errorString() : QStringLiteral("服务对象不存在"));
            finish();
            return;
        }
        m_logView->appendPlainText(QStringLiteral("服务端已在原端口重新启动，等待客户端自动重连…"));
    });
}

void SystemTestDialog::sendTcpRead(quint8 unitId, quint16 transactionId)
{
    if (!m_client->sendReadHoldingRegistersRequest(transactionId, unitId, 0, 3)) {
        fail(QStringLiteral("Modbus TCP请求"), QStringLiteral("客户端拒绝发送读取请求"));
        finish();
    }
}

void SystemTestDialog::handleTcpResponse(const QByteArray &response)
{
    if (m_tcpPhase == TcpPhase::InitialRead || m_tcpPhase == TcpPhase::RestartRead) {
        const bool afterRestart = m_tcpPhase == TcpPhase::RestartRead;
        QVector<quint16> registers;
        QString error;
        const quint16 transaction = afterRestart ? 2 : 1;
        const bool parsed = ModbusTCP::parseReadHoldingRegistersResponse(
            response, 1, transaction, 3, registers, error);
        if (!parsed || registers.size() != 3 || registers[2] != 1) {
            fail(afterRestart ? QStringLiteral("重启后设备数据更新") : QStringLiteral("Modbus TCP读数"),
                 error.isEmpty() ? QStringLiteral("寄存器内容不正确") : error);
            finish();
            return;
        }

        if (!afterRestart) {
            pass(QStringLiteral("Modbus TCP正常读数"),
                 QStringLiteral("温度%1℃，电压%2V，设备在线").arg(registers[0] / 10.0).arg(registers[1]));
            m_tcpPhase = TcpPhase::WaitingDisconnect;
            m_server->stop();
        } else {
            pass(QStringLiteral("服务器重启后数据恢复"),
                 QStringLiteral("重新读取到温度%1℃、电压%2V").arg(registers[0] / 10.0).arg(registers[1]));
            m_tcpPhase = TcpPhase::ExceptionRead;
            sendTcpRead(250, 3);
        }
        return;
    }

    if (m_tcpPhase == TcpPhase::ExceptionRead) {
        QVector<quint16> registers;
        QString error;
        const bool parsed = ModbusTCP::parseReadHoldingRegistersResponse(
            response, 250, 3, 3, registers, error);
        if (!parsed && error.contains(QStringLiteral("Illegal data address")))
            pass(QStringLiteral("非法 Unit ID 异常响应"), error);
        else
            fail(QStringLiteral("非法 Unit ID 异常响应"), error.isEmpty() ? QStringLiteral("预期Modbus异常响应") : error);
        m_tcpPhase = TcpPhase::Done;
        finish();
    }
}

void SystemTestDialog::pass(const QString &name, const QString &detail)
{
    ++m_passCount;
    ++m_completedCount;
    const QString suffix = detail.isEmpty() ? QString() : QStringLiteral(" — ") + detail;
    m_logView->appendPlainText(QStringLiteral("[%1] 通过：%2%3")
        .arg(QDateTime::currentDateTime().toString("HH:mm:ss"), name, suffix));
    qInfo().noquote() << "[自检通过]" << name << detail;
    m_progressBar->setValue(qMin(m_progressBar->maximum(), m_completedCount));
}

void SystemTestDialog::fail(const QString &name, const QString &detail)
{
    ++m_failCount;
    ++m_completedCount;
    m_logView->appendPlainText(QStringLiteral("[%1] 失败：%2 — %3")
        .arg(QDateTime::currentDateTime().toString("HH:mm:ss"), name, detail));
    qWarning().noquote() << "[自检失败]" << name << detail;
    std::fprintf(stderr, "SELF_TEST_FAILURE: %s — %s\n",
                 qPrintable(name), qPrintable(detail));
    std::fflush(stderr);
    m_progressBar->setValue(qMin(m_progressBar->maximum(), m_completedCount));
}

void SystemTestDialog::finish()
{
    if (m_finishing)
        return;

    m_finishing = true;
    if (m_tcpPhase != TcpPhase::Done)
        m_tcpPhase = TcpPhase::Done;

    if (m_server && m_server->isRunning())
        m_server->stop();
    if (m_client)
        m_client->disconnectFromDevice();

    // 等待超过客户端的 3 秒重连周期，确认主动断开后不会再次尝试连接。
    const int tcpErrorsAfterDisconnect = m_tcpErrorCount;
    QTimer::singleShot(3200, this, [this, tcpErrorsAfterDisconnect]() {
        if (m_tcpErrorCount == tcpErrorsAfterDisconnect) {
            pass(QStringLiteral("自检结束后停止 TCP 重连"),
                 QStringLiteral("主动断开后等待 3.2 秒，未出现新的 TCP 连接错误"));
        } else {
            fail(QStringLiteral("自检结束后停止 TCP 重连"),
                 QStringLiteral("主动断开后仍出现新的 TCP 连接错误"));
        }

        m_summaryLabel->setText(QStringLiteral("自检完成：通过 %1，失败 %2。串口现场收发需接入虚拟串口对后单独验证。")
            .arg(m_passCount).arg(m_failCount));
        qInfo() << "一键自检完成，通过:" << m_passCount << "失败:" << m_failCount;
        m_logView->appendPlainText(QStringLiteral("串口现场测试：跳过（避免占用当前 COM 端口；连接 COM1/COM2 虚拟串口对后请手工验证）。"));
        m_runButton->setEnabled(true);
        emit testsFinished(m_passCount, m_failCount);
    });
}
