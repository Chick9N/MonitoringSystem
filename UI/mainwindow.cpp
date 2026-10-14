#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "../UI/devicewidget.h"
#include <QTableWidget>
#include <QHeaderView>
#include <QAbstractItemView>
#include "minichartwidget.h"
#include "mockserialconfigwindow.h"
#include "systemtestdialog.h"
#include <QPushButton>
#include "adddevicedialog.h"
#include <QMessageBox>
#include <QtConcurrent/QtConcurrentRun>
#include <QLabel>
#include <QColor>
#include <QDesktopServices>
#include <QFileInfo>
#include <QUrl>
#include <QBrush>
#include <QIcon>
#include <QLineEdit>
#include "../Utils/applogger.h"
#include <algorithm>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_ui(new Ui::MainWindow)
    , m_databaseManager(new DatabaseManager(this))
    , m_deviceManager(new DeviceManager(m_databaseManager, this))
    , m_databaseWriteQueue(nullptr)
    , m_tcpServerService(createTcpServerService(this))
    , m_updateTimer(new QTimer(this))
{
    m_ui->setupUi(this);
    setWindowIcon(QIcon(QStringLiteral(":/assets/monitoring-app.svg")));
    // 等待数据库设备快照加载完成后再允许启动模拟设备，避免启动时重复加载配置。
    m_ui->startAllBtn->setEnabled(false);
    m_ui->stopAllBtn->setEnabled(false);

    // 连接指示灯反映通信通道状态；设备行则反映是否收到有效数据。
    auto *serialIndicator = new QLabel(QStringLiteral("● 串口未连接"), this);
    auto *tcpClientIndicator = new QLabel(QStringLiteral("● TCP客户端未连接"), this);
    statusBar()->addPermanentWidget(serialIndicator);
    statusBar()->addPermanentWidget(tcpClientIndicator);
    setConnectionIndicator(serialIndicator, false);
    setConnectionIndicator(tcpClientIndicator, false);

    connect(m_deviceManager, &DeviceManager::serialPortOpened, this,
            [this, serialIndicator]() {
        serialIndicator->setText(QStringLiteral("● 串口已连接"));
        setConnectionIndicator(serialIndicator, true);
    });
    connect(m_deviceManager, &DeviceManager::serialPortClosed, this,
            [this, serialIndicator]() {
        serialIndicator->setText(QStringLiteral("● 串口未连接"));
        setConnectionIndicator(serialIndicator, false);
    });
    connect(m_deviceManager, &DeviceManager::serialPortError, this,
            [this, serialIndicator](const QString &message) {
        serialIndicator->setText(QStringLiteral("● 串口错误"));
        serialIndicator->setToolTip(message);
        setConnectionIndicator(serialIndicator, false);
    });
    connect(m_deviceManager, &DeviceManager::tcpConnectionChanged, this,
            [this, tcpClientIndicator](bool connected, const QString &message) {
        tcpClientIndicator->setText(connected ? QStringLiteral("● TCP客户端已连接")
                                               : QStringLiteral("● TCP客户端未连接"));
        tcpClientIndicator->setToolTip(message);
        setConnectionIndicator(tcpClientIndicator, connected);
    });

    m_alarmBlinkTimer = new QTimer(this);
    m_alarmBlinkTimer->setInterval(500);
    connect(m_alarmBlinkTimer, &QTimer::timeout, this, &MainWindow::updateAlarmBlinking);
    m_ui->deviceTable->setColumnCount(9);
    connect(m_ui->deviceSearchEdit, &QLineEdit::textChanged,
            this, &MainWindow::filterDevices);
    m_ui->currentAlarmTable->setColumnCount(5);
    m_ui->currentAlarmTable->setHorizontalHeaderLabels({
        QStringLiteral("设备ID"), QStringLiteral("类型"), QStringLiteral("报警信息"),
        QStringLiteral("时间"), QStringLiteral("确认状态")});
    m_ui->deviceTable->setHorizontalHeaderLabels({
        "设备ID",
        "来源",
        "协议",
        "状态",
        "温度",
        "电压",
        "温度走势图",
        "电压走势图",
        "操作"
    });
    auto *header = m_ui->deviceTable->horizontalHeader();
    header->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(6, QHeaderView::Stretch);
    header->setSectionResizeMode(7, QHeaderView::Stretch);
    header->setSectionResizeMode(8, QHeaderView::ResizeToContents);

    for (QTableWidget *table : {m_ui->alarmTable, m_ui->currentAlarmTable})
    {
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setAlternatingRowColors(true);
        table->verticalHeader()->hide();
        table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
        table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    }
    m_ui->currentAlarmTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);

    // DeviceManager 数据更新 -> MainWindow
    connect(m_deviceManager,
            &DeviceManager::deviceDataUpdated,
            this,
            &MainWindow::updateDeviceUI);

    // 定时更新所有设备
    connect(m_updateTimer,
            &QTimer::timeout,
            m_deviceManager,
            &DeviceManager::updateAllDevices);


    connect(
        m_deviceManager,
        &DeviceManager::deviceRemoved,
        this,
        &MainWindow::removeDeviceRow
        );

    // 数据库
    m_databaseManager->openDatabase();
    m_databaseManager->createTables();
    m_databaseWriteQueue = new DatabaseWriteQueue(
        m_databaseManager->databasePath(), this);

    // 设备添加 -> MainWindow 表格
    connect(
        m_deviceManager,
        &DeviceManager::deviceAdded,
        this,
        &MainWindow::addDeviceRow
        );

    connect(m_tcpServerService, &TcpServerService::stateChanged,
            this, [this](bool running, const QString &message) {
        m_ui->tcpServerBtn->setText(running ? "关闭TCP服务器" : "启动TCP服务器");
        m_ui->tcpStatusLabel->setText(QStringLiteral("● ") + message);
        m_ui->tcpStatusLabel->setStyleSheet(running
            ? QStringLiteral("color:#15803D;font-weight:600")
            : QStringLiteral("color:#64748B;font-weight:600"));
        if (running)
            m_deviceManager->reconnectTCPDevices();
    });
    connect(m_tcpServerService, &TcpServerService::errorOccurred,
            this, [this](const QString &message) {
        m_ui->tcpStatusLabel->setText(QStringLiteral("● TCP服务器启动失败"));
        m_ui->tcpStatusLabel->setStyleSheet(QStringLiteral("color:#DC2626;font-weight:600"));
        QMessageBox::warning(this, "TCP服务器", message);
    });

    m_ui->alarmCountLabel->setStyleSheet(QStringLiteral("font-size:15px;font-weight:700;color:#334155"));
    m_ui->currentAlarmCountLabel->setStyleSheet(QStringLiteral("font-size:15px;font-weight:700;color:#B91C1C"));

    // 启动数据通过工作线程中的独立 SQLite 连接读取；界面更新仍在主线程执行。
    m_startupWatcher = new QFutureWatcher<DatabaseSnapshot>(this);
    connect(m_startupWatcher, &QFutureWatcher<DatabaseSnapshot>::finished,
            this, [this]() {
        applyStartupData(m_startupWatcher->result());
    });

    const QString databasePath = m_databaseManager->databasePath();
    m_startupWatcher->setFuture(QtConcurrent::run([databasePath]() {
        return DatabaseManager::loadStartupSnapshot(databasePath);
    }));

    connect(m_deviceManager, &DeviceManager::deviceDataUpdated,
            this, [this](int deviceId, const DeviceData &data) {
        m_databaseWriteQueue->enqueueDeviceData(deviceId, data);
    });

    connect(
        m_deviceManager,
        &DeviceManager::alarmTriggered,
        this,
        &MainWindow::handleAlarm
        );

}

// 设备表格的生命周期与界面展示。
// 新增和删除表格行时维护设备 ID 与行号映射，供筛选和数据更新使用。
void MainWindow::addDeviceRow(int deviceId)
{
    // 防止重复添加表格行
    for (int row = 0; row < m_ui->deviceTable->rowCount(); ++row)
    {
        auto *item = m_ui->deviceTable->item(row, 0);

        if (item && item->text().toInt() == deviceId)
            return;
    }

    int row = m_ui->deviceTable->rowCount();
    m_deviceRowMap[deviceId] = row;
    m_ui->deviceTable->insertRow(row);
    m_ui->deviceTable->setRowHeight(row, 120);

    m_ui->deviceTable->setItem(
        row, 0,
        new QTableWidgetItem(QString::number(deviceId))
        );

    Device *device = m_deviceManager->getDevice(deviceId);

    if (device && m_ui->deviceTable->item(row, 0))
        m_ui->deviceTable->item(row, 0)->setToolTip(device->config().deviceName);

    QString sourceName = "--";
    QString protocolName = "--";

    // 数据来源：读取设备独立配置
    if (device)
    {
        switch (device->config().dataSource)
        {
        case DataSource::Simulation:
            sourceName = "模拟数据";
            break;

        case DataSource::Serial:
            sourceName = "串口";
            break;

        case DataSource::TCP:
            sourceName = "TCP";
            break;
        }
    }

    m_ui->deviceTable->setItem(
        row, 1,
        new QTableWidgetItem(sourceName)
        );

    m_ui->deviceTable->setItem(
        row, 2,
        new QTableWidgetItem(protocolName)
        );

    m_ui->deviceTable->setItem(
        row, 3,
        new QTableWidgetItem("离线")
        );

    m_ui->deviceTable->setItem(
        row, 4,
        new QTableWidgetItem("--")
        );

    m_ui->deviceTable->setItem(
        row, 5,
        new QTableWidgetItem("--")
        );

    auto *temperatureChart =
        new MiniChartWidget(
            ChartType::Temperature,
            m_ui->deviceTable
            );

    auto *voltageChart =
        new MiniChartWidget(
            ChartType::Voltage,
            m_ui->deviceTable
            );

    m_ui->deviceTable->setCellWidget(
        row, 6, temperatureChart
        );

    m_ui->deviceTable->setCellWidget(
        row, 7, voltageChart
        );

    // 每行独立的删除确认按钮
    auto *deleteBtn = new QPushButton("删除", m_ui->deviceTable);

    connect(
        deleteBtn,
        &QPushButton::clicked,
        this,
        [this, deviceId, deleteBtn]()
        {
            if (deleteBtn->text() == "删除")
            {
                deleteBtn->setText("确认删除");
                return;
            }

            m_deviceManager->removeDevice(deviceId);
        }
        );

    m_ui->deviceTable->setCellWidget(
        row, 8, deleteBtn
        );

    m_temperatureHistory[deviceId] = {};
    m_voltageHistory[deviceId] = {};

    filterDevices(m_ui->deviceSearchEdit->text());

    qDebug() << "MainWindow 新增设备行:" << deviceId;
}

void MainWindow::removeDeviceRow(int deviceId)
{
    if (m_deviceWidgets.contains(deviceId) && m_deviceWidgets.value(deviceId))
        m_deviceWidgets.value(deviceId)->close();

    for (int row = m_ui->alarmTable->rowCount() - 1; row >= 0; --row) {
        const auto *idItem = m_ui->alarmTable->item(row, 0);
        if (idItem && idItem->text().toInt() == deviceId)
            m_ui->alarmTable->removeRow(row);
    }
    for (int row = m_ui->currentAlarmTable->rowCount() - 1; row >= 0; --row) {
        const auto *idItem = m_ui->currentAlarmTable->item(row, 0);
        if (idItem && idItem->text().toInt() == deviceId)
            m_ui->currentAlarmTable->removeRow(row);
    }
    updateAlarmStatistics();
    updateAlarmBlinking();

    if(!m_deviceRowMap.contains(deviceId))
        return;


    int row =
        m_deviceRowMap[deviceId];


    // QTableWidget 删除行后会调整后续行号，因此需要重新建立 ID 与行号映射。
    m_ui->deviceTable->removeRow(row);


    m_deviceRowMap.remove(deviceId);



    // 重新整理行号

    for(auto it=m_deviceRowMap.begin();
         it!=m_deviceRowMap.end();
         ++it)
    {

        if(it.value()>row)
        {
            it.value()--;
        }

    }


    m_temperatureHistory.remove(deviceId);
    m_voltageHistory.remove(deviceId);


    qDebug()
        <<"删除设备UI:"
        <<deviceId;

}

void MainWindow::updateDeviceUI(int deviceId, const DeviceData &data)
{
    qDebug() << "MainWindow 收到数据:"
             << deviceId
             << data.temperature
             << data.voltage
             << data.isOnline;

    if (!m_deviceRowMap.contains(deviceId))
    {
        qWarning() << "设备不存在表格:" << deviceId;
        return;
    }

    int row = m_deviceRowMap[deviceId];

    m_ui->deviceTable->setRowHeight(row, 120);

    // 设备ID
    m_ui->deviceTable->setItem(
        row, 0,
        new QTableWidgetItem(QString::number(deviceId))
        );

    // 获取设备配置
    Device *device = m_deviceManager->getDevice(deviceId);

    QString sourceName = "--";
    QString protocolName = "--";

    if (device)
    {
        const DeviceConfig &config = device->config();

        // 设备名称作为 ID 单元格的悬停提示
        QTableWidgetItem *idItem = m_ui->deviceTable->item(row, 0);
        if (idItem)
        {
            idItem->setToolTip(config.deviceName);
        }

        // 协议类型
        switch (config.protocolType)
        {
        case ProtocolType::Custom:
            protocolName = "自定义串口";
            break;

        case ProtocolType::ModbusRTU:
            protocolName = "Modbus RTU";
            break;

        case ProtocolType::ModbusTCP:
            protocolName = "Modbus TCP";
            break;
        }
    }

    // 数据来源：读取设备独立配置
    if (device)
    {
        switch (device->config().dataSource)
        {
        case DataSource::Simulation:
            sourceName = "模拟数据";
            break;

        case DataSource::Serial:
            sourceName = "串口";
            break;

        case DataSource::TCP:
            sourceName = "TCP";
            break;
        }
    }

    // 更新来源和协议列
    m_ui->deviceTable->setItem(
        row, 1,
        new QTableWidgetItem(sourceName)
        );

    m_ui->deviceTable->setItem(
        row, 2,
        new QTableWidgetItem(protocolName)
        );

    // 状态
    auto *stateItem = new QTableWidgetItem(data.isOnline ? QStringLiteral("● 在线")
                                                          : QStringLiteral("● 离线"));
    stateItem->setForeground(data.isOnline ? QColor(QStringLiteral("#16A34A"))
                                            : QColor(QStringLiteral("#DC2626")));
    stateItem->setToolTip(data.isOnline ? QStringLiteral("最近数据正常")
                                        : QStringLiteral("设备无响应或已断开"));
    m_ui->deviceTable->setItem(row, 3, stateItem);

    // 获取温度曲线控件
    MiniChartWidget *temperatureChart =
        qobject_cast<MiniChartWidget*>(
            m_ui->deviceTable->cellWidget(row, 6)
            );

    if (!temperatureChart)
    {
        temperatureChart = new MiniChartWidget(
            ChartType::Temperature
            );

        m_ui->deviceTable->setCellWidget(
            row, 6, temperatureChart
            );
    }

    // 获取电压曲线控件
    MiniChartWidget *voltageChart =
        qobject_cast<MiniChartWidget*>(
            m_ui->deviceTable->cellWidget(row, 7)
            );

    if (!voltageChart)
    {
        voltageChart = new MiniChartWidget(
            ChartType::Voltage
            );

        m_ui->deviceTable->setCellWidget(
            row, 7, voltageChart
            );
    }

    if (data.isOnline)
    {
        // 温度
        auto *temperatureItem = new QTableWidgetItem(
            QString::number(data.temperature, 'f', 1));
        auto *voltageItem = new QTableWidgetItem(
            QString::number(data.voltage, 'f', 1));
        if (device && (data.temperature < device->config().temperatureLow ||
                       data.temperature > device->config().temperatureHigh))
            temperatureItem->setForeground(QColor(QStringLiteral("#DC2626")));
        if (device && (data.voltage < device->config().voltageLow ||
                       data.voltage > device->config().voltageHigh))
            voltageItem->setForeground(QColor(QStringLiteral("#DC2626")));
        m_ui->deviceTable->setItem(
            row, 4,
            temperatureItem
            );

        // 电压
        m_ui->deviceTable->setItem(
            row, 5,
            voltageItem
            );

        // 保存历史数据
        m_temperatureHistory[deviceId].append(data.temperature);
        m_voltageHistory[deviceId].append(data.voltage);

        // 限制历史数据长度
        if (m_temperatureHistory[deviceId].size() > m_maxHistoryPoints)
        {
            m_temperatureHistory[deviceId].removeFirst();
        }

        if (m_voltageHistory[deviceId].size() > m_maxHistoryPoints)
        {
            m_voltageHistory[deviceId].removeFirst();
        }

        // 更新曲线
        temperatureChart->setData(m_temperatureHistory[deviceId]);
        voltageChart->setData(m_voltageHistory[deviceId]);
    }
    else
    {
        // 离线时保留状态，温度和电压显示占位符
        m_ui->deviceTable->setItem(
            row, 4,
            new QTableWidgetItem("--")
            );

        m_ui->deviceTable->setItem(
            row, 5,
            new QTableWidgetItem("--")
            );
    }
}

// 窗口生命周期与设备级操作。
MainWindow::~MainWindow()
{
    if (m_updateTimer)
        m_updateTimer->stop();
    if (m_startupWatcher && m_startupWatcher->isRunning())
        m_startupWatcher->waitForFinished();
    delete m_databaseWriteQueue;
    m_databaseWriteQueue = nullptr;
    delete m_ui;
}

void MainWindow::on_startAllBtn_clicked()
{
    if (m_simulationRunning)
        return;

    const QList<Device *> currentDevices = m_deviceManager->devices();
    const bool hasSimulationDevice = std::any_of(
        currentDevices.cbegin(),
        currentDevices.cend(),
        [](const Device *device) {
            return device && device->config().dataSource == DataSource::Simulation;
        });

    if (!hasSimulationDevice) {
        DeviceConfig config;
        config.deviceName = QStringLiteral("内部模拟设备");
        config.dataSource = DataSource::Simulation;
        config.protocolType = ProtocolType::Custom;

        const int deviceId = m_databaseManager->insertDevice(config);
        if (deviceId <= 0) {
            QMessageBox::critical(this, QStringLiteral("启动模拟设备失败"),
                                  QStringLiteral("无法保存内部模拟设备配置。"));
            return;
        }

        config.deviceId = deviceId;
        auto *device = new Device(deviceId, m_deviceManager);
        device->setConfig(config);
        if (!m_deviceManager->addDevice(device)) {
            m_databaseManager->deleteDevice(deviceId);
            device->deleteLater();
            QMessageBox::critical(this, QStringLiteral("启动模拟设备失败"),
                                  QStringLiteral("内部模拟设备初始化失败，已回滚配置。"));
            return;
        }
    }

    m_deviceManager->startSimulationDevices();
    m_simulationRunning = true;
    m_ui->startAllBtn->setEnabled(false);
    m_ui->stopAllBtn->setEnabled(true);
}


void MainWindow::on_stopAllBtn_clicked()
{
    if (!m_simulationRunning)
        return;

    m_deviceManager->stopSimulationDevices();
    m_simulationRunning = false;
    m_ui->startAllBtn->setEnabled(true);
    m_ui->stopAllBtn->setEnabled(false);
}

void MainWindow::on_deviceTable_cellDoubleClicked(int row, int column)
{
    Q_UNUSED(column);

    if (row < 0 || row >= m_ui->deviceTable->rowCount())
        return;

    QTableWidgetItem *idItem =
        m_ui->deviceTable->item(row, 0);

    if (!idItem)
        return;

    int deviceId = idItem->text().toInt();

    Device *device = m_deviceManager->getDevice(deviceId);

    if (!device)
        return;

    if(m_deviceWidgets.contains(deviceId)){
        m_deviceWidgets[deviceId]->raise(); // 把窗口提到其他窗口前面
        m_deviceWidgets[deviceId]->activateWindow(); // 让这个窗口成为当前活动窗口
        return;
    }

    DeviceWidget *widget = new DeviceWidget(device, m_databaseManager, this); // 挂到父窗口下, 否则mainwindow销毁widget不销毁

    m_deviceWidgets[deviceId] = widget;

    // 窗口关闭时从已打开窗口管理表(QMap)中移除
    connect(widget,
            &QObject::destroyed,
            [this,deviceId](){
                m_deviceWidgets.remove(deviceId);
            });

    widget->show();
}

// 报警历史、当前报警状态和确认操作。
void MainWindow::handleAlarm(const AlarmInfo &alarm)
{
    // 保存报警记录
    if (m_databaseWriteQueue)
        m_databaseWriteQueue->enqueueAlarm(alarm);

    // 1. 记录报警事件
    int row = m_ui->alarmTable->rowCount();

    m_ui->alarmTable->insertRow(row);

    m_ui->alarmTable->setItem(
        row,
        0,
        new QTableWidgetItem(
            QString::number(alarm.deviceId)
            )
        );
    m_ui->alarmTable->setItem(
        row,
        1,
        new QTableWidgetItem(
            alarm.recovered ? "恢复" : "报警"
            )
        );
    m_ui->alarmTable->item(row, 1)->setForeground(
        alarm.recovered ? QColor(QStringLiteral("#15803D")) : QColor(QStringLiteral("#B91C1C")));

    m_ui->alarmTable->setItem(
        row,
        2,
        new QTableWidgetItem(alarm.message)
        );

    m_ui->alarmTable->setItem(
        row,
        3,
        new QTableWidgetItem(
            alarm.timestamp.toString("HH:mm:ss")
            )
        );
    if (m_ui->alarmTable->rowCount() > 500)
        m_ui->alarmTable->removeRow(0);

    // 2. 更新当前报警状态

    if (!alarm.recovered)
    {
        // 防止重复加入
        if (findCurrentAlarm(alarm.deviceId, alarm.type) == -1)
        {
            int currentRow = m_ui->currentAlarmTable->rowCount();

            m_ui->currentAlarmTable->insertRow(currentRow);

            // 设备ID
            m_ui->currentAlarmTable->setItem(
                currentRow,
                0,
                new QTableWidgetItem(
                    QString::number(alarm.deviceId)
                    )
                );

            // 报警类型
            auto *typeItem = new QTableWidgetItem(alarmTypeToString(alarm.type));

            typeItem->setData(
                Qt::UserRole,
                static_cast<int>(alarm.type)
                );

            m_ui->currentAlarmTable->setItem(
                currentRow,
                1,
                typeItem
                );

            // 报警信息
            m_ui->currentAlarmTable->setItem(
                currentRow,
                2,
                new QTableWidgetItem(alarm.message)
                );

            // 发生时间
            m_ui->currentAlarmTable->setItem(
                currentRow,
                3,
                new QTableWidgetItem(
                    alarm.timestamp.toString("HH:mm:ss")
                    )
                );
            m_ui->currentAlarmTable->setItem(
                currentRow, 4,
                new QTableWidgetItem(alarm.acknowledged ? QStringLiteral("已确认")
                                                         : QStringLiteral("未确认")));
        }
    }
    else
    {
        int row = findCurrentAlarm(
            alarm.deviceId,
            alarm.type
            );

        if (row != -1)
        {
            m_ui->currentAlarmTable->removeRow(row);
        }
    }

    // 更新统计数字
    updateAlarmStatistics();
    updateAlarmBlinking();
}

void MainWindow::updateAlarmBlinking()
{
    bool hasUnacknowledged = false;
    for (int row = 0; row < m_ui->currentAlarmTable->rowCount(); ++row) {
        const auto *ackItem = m_ui->currentAlarmTable->item(row, 4);
        if (!ackItem || ackItem->text() != QStringLiteral("已确认")) {
            hasUnacknowledged = true;
            break;
        }
    }
    if (!hasUnacknowledged) {
        m_alarmBlinkTimer->stop();
        m_alarmBlinkPhase = false;
        for (int row = 0; row < m_ui->currentAlarmTable->rowCount(); ++row)
            for (int column = 0; column < m_ui->currentAlarmTable->columnCount(); ++column)
                if (auto *item = m_ui->currentAlarmTable->item(row, column))
                    item->setBackground(QBrush());
        return;
    }
    if (!m_alarmBlinkTimer->isActive())
        m_alarmBlinkTimer->start();
    m_alarmBlinkPhase = !m_alarmBlinkPhase;
    const QColor alertColor = m_alarmBlinkPhase ? QColor(QStringLiteral("#FEE2E2")) : QColor(QStringLiteral("#FFF7F7"));
    for (int row = 0; row < m_ui->currentAlarmTable->rowCount(); ++row) {
        const auto *ackItem = m_ui->currentAlarmTable->item(row, 4);
        const bool unacknowledged = !ackItem || ackItem->text() != QStringLiteral("已确认");
        for (int column = 0; column < m_ui->currentAlarmTable->columnCount(); ++column) {
            if (auto *item = m_ui->currentAlarmTable->item(row, column))
                item->setBackground(unacknowledged ? alertColor : QBrush());
        }
    }
}

void MainWindow::setConnectionIndicator(QLabel *indicator, bool active)
{
    if (!indicator)
        return;
    indicator->setStyleSheet(active
        ? QStringLiteral("color:#16A34A;font-weight:600;padding:0 6px")
        : QStringLiteral("color:#DC2626;font-weight:600;padding:0 6px"));
}

// 设备搜索与报警表格辅助函数。
void MainWindow::filterDevices(const QString &query)
{
    const QString normalized = query.trimmed();
    for (int row = 0; row < m_ui->deviceTable->rowCount(); ++row) {
        bool matches = normalized.isEmpty();
        for (int column = 0; !matches && column < m_ui->deviceTable->columnCount(); ++column) {
            const QTableWidgetItem *item = m_ui->deviceTable->item(row, column);
            if (item && (item->text().contains(normalized, Qt::CaseInsensitive) ||
                         item->toolTip().contains(normalized, Qt::CaseInsensitive)))
                matches = true;
        }
        m_ui->deviceTable->setRowHidden(row, !matches);
    }
}

int MainWindow::findCurrentAlarm(
    int deviceId,
    AlarmType type) const
{
    for (int row = 0;
         row < m_ui->currentAlarmTable->rowCount();
         ++row)
    {
        const auto *idItem = m_ui->currentAlarmTable->item(row, 0);
        const auto *typeItem = m_ui->currentAlarmTable->item(row, 1);
        if (!idItem || !typeItem)
            continue;
        const int id = idItem->text().toInt();

        AlarmType currentType =
            static_cast<AlarmType>(
                typeItem->data(Qt::UserRole)
                    .toInt()
                );

        if (id == deviceId && currentType == type)
            return row;
    }

    return -1;
}

void MainWindow::updateAlarmStatistics()
{
    m_ui->alarmCountLabel->setText(
        QString("最近报警：%1")
            .arg(m_ui->alarmTable->rowCount())
        );

    m_ui->currentAlarmCountLabel->setText(
        QString("当前报警：%1")
            .arg(m_ui->currentAlarmTable->rowCount())
        );
}


void MainWindow::on_mockSerialConfigBtn_clicked()
{
    if (!m_mockSerialConfigWindow)
        m_mockSerialConfigWindow = new MockSerialConfigWindow(this);

    m_mockSerialConfigWindow->show();
    m_mockSerialConfigWindow->raise();
    m_mockSerialConfigWindow->activateWindow();
}

// 管理器发出 deviceAdded 信号后会先创建表格行，再显示设备的首笔数据。
void MainWindow::on_addDeviceBtn_clicked()
{
    AddDeviceDialog dialog(this);


    connect(
        &dialog,
        &AddDeviceDialog::testTCPConnection,
        m_deviceManager,
        &DeviceManager::testTCPConnection
        );

    connect(
        m_deviceManager,
        &DeviceManager::tcpTestResult,
        &dialog,
        &AddDeviceDialog::onTCPTestResult
        );

    if(dialog.exec() != QDialog::Accepted)
    {
        return;
    }


    DeviceConfig config =
        dialog.getDeviceConfig();

    // 先写入数据库获取设备 ID；若通信初始化失败，则回滚刚写入的配置。
    int deviceId =
        m_databaseManager->insertDevice(config);

    if (deviceId <= 0)
    {
        QMessageBox::critical(
            this,
            "添加设备失败",
            "数据库无法分配设备 ID。"
            );

        return;
    }

    // 将数据库生成的 ID 写回配置
    config.deviceId = deviceId;

    // 使用数据库生成的 ID 创建 Device
    Device *device =
        new Device(
            deviceId,
            m_deviceManager
            );

    device->setConfig(config);

    if (!m_deviceManager->addDevice(device))
    {
        m_databaseManager->deleteDevice(deviceId);

        QMessageBox::critical(
            this,
            "添加设备失败",
            "设备初始化失败，已回滚数据库记录。"
            );

        return;
    }
}

// 模拟器控制与诊断功能。
void MainWindow::on_tcpServerBtn_clicked()
{
    if (m_tcpServerService->isRunning())
        m_tcpServerService->stop();
    else
        m_tcpServerService->start(1502);
}

void MainWindow::on_runSelfTestBtn_clicked()
{
    if (!m_systemTestDialog) {
        m_systemTestDialog = new SystemTestDialog(this);
        m_systemTestDialog->setAttribute(Qt::WA_DeleteOnClose);
    }
    m_systemTestDialog->show();
    m_systemTestDialog->raise();
    m_systemTestDialog->activateWindow();
    QMetaObject::invokeMethod(m_systemTestDialog, "runAllTests", Qt::QueuedConnection);
}

void MainWindow::on_openLogsBtn_clicked()
{
    const QString logDirectory = QFileInfo(AppLogger::logFilePath()).absolutePath();
    QDesktopServices::openUrl(QUrl::fromLocalFile(logDirectory));
}

void MainWindow::on_acknowledgeAlarmBtn_clicked()
{
    const int row = m_ui->currentAlarmTable->currentRow();
    if (row < 0)
        return;
    auto *deviceItem = m_ui->currentAlarmTable->item(row, 0);
    auto *typeItem = m_ui->currentAlarmTable->item(row, 1);
    if (!deviceItem || !typeItem)
        return;
    const int deviceId = deviceItem->text().toInt();
    const auto type = static_cast<AlarmType>(typeItem->data(Qt::UserRole).toInt());
    if (auto *ackItem = m_ui->currentAlarmTable->item(row, 4)) {
        ackItem->setText(QStringLiteral("已确认"));
        ackItem->setForeground(QColor(QStringLiteral("#15803D")));
    }
    if (m_databaseWriteQueue)
        m_databaseWriteQueue->enqueueAlarmAcknowledgement(deviceId, type);
    updateAlarmBlinking();
}

void MainWindow::applyStartupData(const DatabaseSnapshot &data)
{
    for (const DeviceConfig &config : data.devices) {
        Device *device = new Device(config.deviceId, m_deviceManager);
        device->setConfig(config);
        m_deviceManager->addDevice(device);
    }

    for (const AlarmInfo &alarm : data.alarmHistory) {
        int row = m_ui->alarmTable->rowCount();
        m_ui->alarmTable->insertRow(row);
        m_ui->alarmTable->setItem(row, 0, new QTableWidgetItem(QString::number(alarm.deviceId)));
        m_ui->alarmTable->setItem(row, 1, new QTableWidgetItem(alarm.recovered ? "恢复" : "报警"));
        m_ui->alarmTable->setItem(row, 2, new QTableWidgetItem(alarm.message));
        m_ui->alarmTable->setItem(row, 3, new QTableWidgetItem(alarm.timestamp.toString("HH:mm:ss")));
        if (auto *typeItem = m_ui->alarmTable->item(row, 1))
            typeItem->setForeground(alarm.recovered ? QColor(QStringLiteral("#15803D"))
                                                     : QColor(QStringLiteral("#B91C1C")));
    }

    for (const AlarmInfo &alarm : data.activeAlarms) {
        int row = m_ui->currentAlarmTable->rowCount();
        m_ui->currentAlarmTable->insertRow(row);
        m_ui->currentAlarmTable->setItem(row, 0, new QTableWidgetItem(QString::number(alarm.deviceId)));
        auto *typeItem = new QTableWidgetItem(alarmTypeToString(alarm.type));
        typeItem->setData(Qt::UserRole, static_cast<int>(alarm.type));
        m_ui->currentAlarmTable->setItem(row, 1, typeItem);
        m_ui->currentAlarmTable->setItem(row, 2, new QTableWidgetItem(alarm.message));
        m_ui->currentAlarmTable->setItem(row, 3, new QTableWidgetItem(alarm.timestamp.toString("HH:mm:ss")));
        auto *ackItem = new QTableWidgetItem(alarm.acknowledged ? QStringLiteral("已确认")
                                                                : QStringLiteral("未确认"));
        if (alarm.acknowledged)
            ackItem->setForeground(QColor(QStringLiteral("#15803D")));
        m_ui->currentAlarmTable->setItem(row, 4, ackItem);
    }

    if (!data.activeAlarms.isEmpty())
        m_alarmBlinkTimer->start();
    m_deviceManager->restoreActiveAlarms(data.activeAlarms);
    updateAlarmBlinking();

    qDebug() << "后台加载完成：设备" << data.devices.size()
             << "条，历史报警" << data.alarmHistory.size()
             << "条，当前报警" << data.activeAlarms.size() << "条";
    m_deviceManager->startPolling();
    m_ui->startAllBtn->setEnabled(true);
    if (m_updateTimer && !m_updateTimer->isActive())
        m_updateTimer->start(1000);
}
