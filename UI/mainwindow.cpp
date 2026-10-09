#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "../UI/devicewidget.h"
#include <QTableWidget>
#include <QHeaderView>
#include <QAbstractItemView>
#include "minichartwidget.h"
#include "mockserialconfigwindow.h"
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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , databaseManager(new DatabaseManager(this))
    , databaseWriteQueue(nullptr)
    , timer(new QTimer(this))
    , m_tcpServerService(createTcpServerService(this))
{
    deviceManager= new DeviceManager(databaseManager, this);

    ui->setupUi(this);
    setWindowIcon(QIcon(QStringLiteral(":/assets/monitoring-app.svg")));
    auto *serialIndicator = new QLabel(QStringLiteral("● 串口未连接"), this);
    auto *tcpClientIndicator = new QLabel(QStringLiteral("● TCP客户端未连接"), this);
    statusBar()->addPermanentWidget(serialIndicator);
    statusBar()->addPermanentWidget(tcpClientIndicator);
    setConnectionIndicator(serialIndicator, false);
    setConnectionIndicator(tcpClientIndicator, false);

    connect(deviceManager, &DeviceManager::serialPortOpened, this,
            [this, serialIndicator]() {
        serialIndicator->setText(QStringLiteral("● 串口已连接"));
        setConnectionIndicator(serialIndicator, true);
    });
    connect(deviceManager, &DeviceManager::serialPortClosed, this,
            [this, serialIndicator]() {
        serialIndicator->setText(QStringLiteral("● 串口未连接"));
        setConnectionIndicator(serialIndicator, false);
    });
    connect(deviceManager, &DeviceManager::serialPortError, this,
            [this, serialIndicator](const QString &message) {
        serialIndicator->setText(QStringLiteral("● 串口错误"));
        serialIndicator->setToolTip(message);
        setConnectionIndicator(serialIndicator, false);
    });
    connect(deviceManager, &DeviceManager::tcpConnectionChanged, this,
            [this, tcpClientIndicator](bool connected, const QString &message) {
        tcpClientIndicator->setText(connected ? QStringLiteral("● TCP客户端已连接")
                                               : QStringLiteral("● TCP客户端未连接"));
        tcpClientIndicator->setToolTip(message);
        setConnectionIndicator(tcpClientIndicator, connected);
    });

    m_alarmBlinkTimer = new QTimer(this);
    m_alarmBlinkTimer->setInterval(500);
    connect(m_alarmBlinkTimer, &QTimer::timeout, this, &MainWindow::updateAlarmBlinking);
    ui->deviceTable->setColumnCount(9);
    connect(ui->deviceSearchEdit, &QLineEdit::textChanged,
            this, &MainWindow::filterDevices);
    ui->currentAlarmTable->setColumnCount(5);
    ui->currentAlarmTable->setHorizontalHeaderLabels({
        QStringLiteral("设备ID"), QStringLiteral("类型"), QStringLiteral("报警信息"),
        QStringLiteral("时间"), QStringLiteral("确认状态")});
    ui->deviceTable->setHorizontalHeaderLabels({
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
    auto *header = ui->deviceTable->horizontalHeader();
    header->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(6, QHeaderView::Stretch);
    header->setSectionResizeMode(7, QHeaderView::Stretch);
    header->setSectionResizeMode(8, QHeaderView::ResizeToContents);

    for (QTableWidget *table : {ui->alarmTable, ui->currentAlarmTable})
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
    ui->currentAlarmTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);

    // DeviceManager 数据更新 -> MainWindow
    connect(deviceManager,
            &DeviceManager::deviceDataUpdated,
            this,
            &MainWindow::updateDeviceUI);

    // 定时更新所有设备
    connect(timer,
            &QTimer::timeout,
            deviceManager,
            &DeviceManager::updateAllDevices);


    connect(
        deviceManager,
        &DeviceManager::deviceRemoved,
        this,
        &MainWindow::removeDeviceRow
        );

    // 数据库
    databaseManager->openDatabase();
    databaseManager->createTables();
    databaseWriteQueue = new DatabaseWriteQueue(
        databaseManager->databasePath(), this);

    // 设备添加 -> MainWindow 表格
    connect(
        deviceManager,
        &DeviceManager::deviceAdded,
        this,
        &MainWindow::addDeviceRow
        );

    connect(m_tcpServerService, &TcpServerService::stateChanged,
            this, [this](bool running, const QString &message) {
        ui->tcpServerBtn->setText(running ? "关闭TCP服务器" : "启动TCP服务器");
        ui->tcpStatusLabel->setText(QStringLiteral("● ") + message);
        ui->tcpStatusLabel->setStyleSheet(running
            ? QStringLiteral("color:#15803D;font-weight:600")
            : QStringLiteral("color:#64748B;font-weight:600"));
        if (running)
            deviceManager->reconnectTCPDevices();
    });
    connect(m_tcpServerService, &TcpServerService::errorOccurred,
            this, [this](const QString &message) {
        ui->tcpStatusLabel->setText(QStringLiteral("● TCP服务器启动失败"));
        ui->tcpStatusLabel->setStyleSheet(QStringLiteral("color:#DC2626;font-weight:600"));
        QMessageBox::warning(this, "TCP服务器", message);
    });

    ui->alarmCountLabel->setStyleSheet(QStringLiteral("font-size:15px;font-weight:700;color:#334155"));
    ui->currentAlarmCountLabel->setStyleSheet(QStringLiteral("font-size:15px;font-weight:700;color:#B91C1C"));

    // 启动时的数据库查询放入后台线程；该线程使用独立 SQLite 连接。
    m_startupWatcher = new QFutureWatcher<DatabaseSnapshot>(this);
    connect(m_startupWatcher, &QFutureWatcher<DatabaseSnapshot>::finished,
            this, [this]() {
        applyStartupData(m_startupWatcher->result());
    });

    const QString databasePath = databaseManager->databasePath();
    m_startupWatcher->setFuture(QtConcurrent::run([databasePath]() {
        return DatabaseManager::loadStartupSnapshot(databasePath);
    }));

    connect(deviceManager, &DeviceManager::deviceDataUpdated,
            this, [this](int deviceId, const DeviceData &data) {
        databaseWriteQueue->enqueueDeviceData(deviceId, data);
    });

    connect(
        deviceManager,
        &DeviceManager::alarmTriggered,
        this,
        &MainWindow::handleAlarm
        );

}

void MainWindow::removeDeviceRow(int deviceId)
{
    if (m_deviceWidgets.contains(deviceId) && m_deviceWidgets.value(deviceId))
        m_deviceWidgets.value(deviceId)->close();

    for (int row = ui->alarmTable->rowCount() - 1; row >= 0; --row) {
        const auto *idItem = ui->alarmTable->item(row, 0);
        if (idItem && idItem->text().toInt() == deviceId)
            ui->alarmTable->removeRow(row);
    }
    for (int row = ui->currentAlarmTable->rowCount() - 1; row >= 0; --row) {
        const auto *idItem = ui->currentAlarmTable->item(row, 0);
        if (idItem && idItem->text().toInt() == deviceId)
            ui->currentAlarmTable->removeRow(row);
    }
    updateAlarmStatistics();
    updateAlarmBlinking();

    if(!m_deviceRowMap.contains(deviceId))
        return;


    int row =
        m_deviceRowMap[deviceId];


    ui->deviceTable->removeRow(row);


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

    ui->deviceTable->setRowHeight(row, 120);

    // 设备ID
    ui->deviceTable->setItem(
        row, 0,
        new QTableWidgetItem(QString::number(deviceId))
        );

    // 获取设备配置
    Device *device = deviceManager->getDevice(deviceId);

    QString sourceName = "--";
    QString protocolName = "--";

    if (device)
    {
        const DeviceConfig &config = device->config();

        // 设备名称作为 ID 单元格的悬停提示
        QTableWidgetItem *idItem = ui->deviceTable->item(row, 0);
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
    ui->deviceTable->setItem(
        row, 1,
        new QTableWidgetItem(sourceName)
        );

    ui->deviceTable->setItem(
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
    ui->deviceTable->setItem(row, 3, stateItem);

    // 获取温度曲线控件
    MiniChartWidget *temperatureChart =
        qobject_cast<MiniChartWidget*>(
            ui->deviceTable->cellWidget(row, 6)
            );

    if (!temperatureChart)
    {
        temperatureChart = new MiniChartWidget(
            ChartType::Temperature
            );

        ui->deviceTable->setCellWidget(
            row, 6, temperatureChart
            );
    }

    // 获取电压曲线控件
    MiniChartWidget *voltageChart =
        qobject_cast<MiniChartWidget*>(
            ui->deviceTable->cellWidget(row, 7)
            );

    if (!voltageChart)
    {
        voltageChart = new MiniChartWidget(
            ChartType::Voltage
            );

        ui->deviceTable->setCellWidget(
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
        ui->deviceTable->setItem(
            row, 4,
            temperatureItem
            );

        // 电压
        ui->deviceTable->setItem(
            row, 5,
            voltageItem
            );

        // 保存历史数据
        m_temperatureHistory[deviceId].append(data.temperature);
        m_voltageHistory[deviceId].append(data.voltage);

        // 限制历史数据长度
        if (m_temperatureHistory[deviceId].size() > MaxHistoryPoints)
        {
            m_temperatureHistory[deviceId].removeFirst();
        }

        if (m_voltageHistory[deviceId].size() > MaxHistoryPoints)
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
        ui->deviceTable->setItem(
            row, 4,
            new QTableWidgetItem("--")
            );

        ui->deviceTable->setItem(
            row, 5,
            new QTableWidgetItem("--")
            );
    }
}


MainWindow::~MainWindow()
{
    if (timer)
        timer->stop();
    if (m_startupWatcher && m_startupWatcher->isRunning())
        m_startupWatcher->waitForFinished();
    delete databaseWriteQueue;
    databaseWriteQueue = nullptr;
    delete ui;
}

void MainWindow::on_startAllBtn_clicked()
{
    deviceManager->startAll();
}


void MainWindow::on_stopAllBtn_clicked()
{
    deviceManager->stopAll();
}

void MainWindow::on_deviceTable_cellDoubleClicked(int row, int column)
{
    Q_UNUSED(column);

    if (row < 0 || row >= ui->deviceTable->rowCount())
        return;

    QTableWidgetItem *idItem =
        ui->deviceTable->item(row, 0);

    if (!idItem)
        return;

    int deviceId = idItem->text().toInt();

    Device *device = deviceManager->getDevice(deviceId);

    if (!device)
        return;

    if(m_deviceWidgets.contains(deviceId)){
        m_deviceWidgets[deviceId]->raise(); // 把窗口提到其他窗口前面
        m_deviceWidgets[deviceId]->activateWindow(); // 让这个窗口成为当前活动窗口
        return;
    }

    DeviceWidget *widget = new DeviceWidget(device, databaseManager, this); // 挂到父窗口下, 否则mainwindow销毁widget不销毁

    m_deviceWidgets[deviceId] = widget;

    // 窗口关闭时从已打开窗口管理表(QMap)中移除
    connect(widget,
            &QObject::destroyed,
            [this,deviceId](){
                m_deviceWidgets.remove(deviceId);
            });

    widget->show();
}

void MainWindow::handleAlarm(const AlarmInfo &alarm)
{
    // 保存报警记录
    if (databaseWriteQueue)
        databaseWriteQueue->enqueueAlarm(alarm);

    // 1. 记录报警事件
    int row = ui->alarmTable->rowCount();

    ui->alarmTable->insertRow(row);

    ui->alarmTable->setItem(
        row,
        0,
        new QTableWidgetItem(
            QString::number(alarm.deviceId)
            )
        );
    ui->alarmTable->setItem(
        row,
        1,
        new QTableWidgetItem(
            alarm.recovered ? "恢复" : "报警"
            )
        );
    ui->alarmTable->item(row, 1)->setForeground(
        alarm.recovered ? QColor(QStringLiteral("#15803D")) : QColor(QStringLiteral("#B91C1C")));

    ui->alarmTable->setItem(
        row,
        2,
        new QTableWidgetItem(alarm.message)
        );

    ui->alarmTable->setItem(
        row,
        3,
        new QTableWidgetItem(
            alarm.timestamp.toString("HH:mm:ss")
            )
        );
    if (ui->alarmTable->rowCount() > 500)
        ui->alarmTable->removeRow(0);

    // 2. 更新当前报警状态

    if (!alarm.recovered)
    {
        // 防止重复加入
        if (findCurrentAlarm(alarm.deviceId, alarm.type) == -1)
        {
            int currentRow = ui->currentAlarmTable->rowCount();

            ui->currentAlarmTable->insertRow(currentRow);

            // 设备ID
            ui->currentAlarmTable->setItem(
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

            ui->currentAlarmTable->setItem(
                currentRow,
                1,
                typeItem
                );

            // 报警信息
            ui->currentAlarmTable->setItem(
                currentRow,
                2,
                new QTableWidgetItem(alarm.message)
                );

            // 发生时间
            ui->currentAlarmTable->setItem(
                currentRow,
                3,
                new QTableWidgetItem(
                    alarm.timestamp.toString("HH:mm:ss")
                    )
                );
            ui->currentAlarmTable->setItem(
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
            ui->currentAlarmTable->removeRow(row);
        }
    }

    // 更新统计数字
    updateAlarmStatistics();
    updateAlarmBlinking();
}

void MainWindow::updateAlarmBlinking()
{
    bool hasUnacknowledged = false;
    for (int row = 0; row < ui->currentAlarmTable->rowCount(); ++row) {
        const auto *ackItem = ui->currentAlarmTable->item(row, 4);
        if (!ackItem || ackItem->text() != QStringLiteral("已确认")) {
            hasUnacknowledged = true;
            break;
        }
    }
    if (!hasUnacknowledged) {
        m_alarmBlinkTimer->stop();
        m_alarmBlinkPhase = false;
        for (int row = 0; row < ui->currentAlarmTable->rowCount(); ++row)
            for (int column = 0; column < ui->currentAlarmTable->columnCount(); ++column)
                if (auto *item = ui->currentAlarmTable->item(row, column))
                    item->setBackground(QBrush());
        return;
    }
    if (!m_alarmBlinkTimer->isActive())
        m_alarmBlinkTimer->start();
    m_alarmBlinkPhase = !m_alarmBlinkPhase;
    const QColor alertColor = m_alarmBlinkPhase ? QColor(QStringLiteral("#FEE2E2")) : QColor(QStringLiteral("#FFF7F7"));
    for (int row = 0; row < ui->currentAlarmTable->rowCount(); ++row) {
        const auto *ackItem = ui->currentAlarmTable->item(row, 4);
        const bool unacknowledged = !ackItem || ackItem->text() != QStringLiteral("已确认");
        for (int column = 0; column < ui->currentAlarmTable->columnCount(); ++column) {
            if (auto *item = ui->currentAlarmTable->item(row, column))
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

void MainWindow::filterDevices(const QString &query)
{
    const QString normalized = query.trimmed();
    for (int row = 0; row < ui->deviceTable->rowCount(); ++row) {
        bool matches = normalized.isEmpty();
        for (int column = 0; !matches && column < ui->deviceTable->columnCount(); ++column) {
            const QTableWidgetItem *item = ui->deviceTable->item(row, column);
            if (item && (item->text().contains(normalized, Qt::CaseInsensitive) ||
                         item->toolTip().contains(normalized, Qt::CaseInsensitive)))
                matches = true;
        }
        ui->deviceTable->setRowHidden(row, !matches);
    }
}

int MainWindow::findCurrentAlarm(
    int deviceId,
    AlarmType type) const
{
    for (int row = 0;
         row < ui->currentAlarmTable->rowCount();
         ++row)
    {
        const auto *idItem = ui->currentAlarmTable->item(row, 0);
        const auto *typeItem = ui->currentAlarmTable->item(row, 1);
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
    ui->alarmCountLabel->setText(
        QString("最近报警：%1")
            .arg(ui->alarmTable->rowCount())
        );

    ui->currentAlarmCountLabel->setText(
        QString("当前报警：%1")
            .arg(ui->currentAlarmTable->rowCount())
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

void MainWindow::addDeviceRow(int deviceId)
{
    // 防止重复添加表格行
    for (int row = 0; row < ui->deviceTable->rowCount(); ++row)
    {
        auto *item = ui->deviceTable->item(row, 0);

        if (item && item->text().toInt() == deviceId)
            return;
    }

    int row = ui->deviceTable->rowCount();
    m_deviceRowMap[deviceId] = row;
    ui->deviceTable->insertRow(row);
    ui->deviceTable->setRowHeight(row, 120);

    ui->deviceTable->setItem(
        row, 0,
        new QTableWidgetItem(QString::number(deviceId))
        );

    Device *device = deviceManager->getDevice(deviceId);

    if (device && ui->deviceTable->item(row, 0))
        ui->deviceTable->item(row, 0)->setToolTip(device->config().deviceName);

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

    ui->deviceTable->setItem(
        row, 1,
        new QTableWidgetItem(sourceName)
        );

    ui->deviceTable->setItem(
        row, 2,
        new QTableWidgetItem(protocolName)
        );

    ui->deviceTable->setItem(
        row, 3,
        new QTableWidgetItem("离线")
        );

    ui->deviceTable->setItem(
        row, 4,
        new QTableWidgetItem("--")
        );

    ui->deviceTable->setItem(
        row, 5,
        new QTableWidgetItem("--")
        );

    auto *temperatureChart =
        new MiniChartWidget(
            ChartType::Temperature,
            ui->deviceTable
            );

    auto *voltageChart =
        new MiniChartWidget(
            ChartType::Voltage,
            ui->deviceTable
            );

    ui->deviceTable->setCellWidget(
        row, 6, temperatureChart
        );

    ui->deviceTable->setCellWidget(
        row, 7, voltageChart
        );

    // 每行独立的删除确认按钮
    auto *deleteBtn = new QPushButton("删除", ui->deviceTable);

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

            deviceManager->removeDevice(deviceId);
        }
        );

    ui->deviceTable->setCellWidget(
        row, 8, deleteBtn
        );

    m_temperatureHistory[deviceId] = {};
    m_voltageHistory[deviceId] = {};

    filterDevices(ui->deviceSearchEdit->text());

    qDebug() << "MainWindow 新增设备行:" << deviceId;
}

void MainWindow::on_addDeviceBtn_clicked()
{
    AddDeviceDialog dialog(this);


    connect(
        &dialog,
        &AddDeviceDialog::testTCPConnection,
        deviceManager,
        &DeviceManager::testTCPConnection
        );

    connect(
        deviceManager,
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

    // 由数据库自动分配设备 ID
    int deviceId =
        databaseManager->insertDevice(config);

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
            deviceManager
            );

    device->setConfig(config);

    if (!deviceManager->addDevice(device))
    {
        databaseManager->deleteDevice(deviceId);

        QMessageBox::critical(
            this,
            "添加设备失败",
            "设备初始化失败，已回滚数据库记录。"
            );

        return;
    }
}

void MainWindow::on_tcpServerBtn_clicked()
{
    if (m_tcpServerService->isRunning())
        m_tcpServerService->stop();
    else
        m_tcpServerService->start(1502);
}

void MainWindow::on_openLogsBtn_clicked()
{
    const QString logDirectory = QFileInfo(AppLogger::logFilePath()).absolutePath();
    QDesktopServices::openUrl(QUrl::fromLocalFile(logDirectory));
}

void MainWindow::on_acknowledgeAlarmBtn_clicked()
{
    const int row = ui->currentAlarmTable->currentRow();
    if (row < 0)
        return;
    auto *deviceItem = ui->currentAlarmTable->item(row, 0);
    auto *typeItem = ui->currentAlarmTable->item(row, 1);
    if (!deviceItem || !typeItem)
        return;
    const int deviceId = deviceItem->text().toInt();
    const auto type = static_cast<AlarmType>(typeItem->data(Qt::UserRole).toInt());
    if (auto *ackItem = ui->currentAlarmTable->item(row, 4)) {
        ackItem->setText(QStringLiteral("已确认"));
        ackItem->setForeground(QColor(QStringLiteral("#15803D")));
    }
    if (databaseWriteQueue)
        databaseWriteQueue->enqueueAlarmAcknowledgement(deviceId, type);
    updateAlarmBlinking();
}

void MainWindow::applyStartupData(const DatabaseSnapshot &data)
{
    for (const DeviceConfig &config : data.devices) {
        Device *device = new Device(config.deviceId, deviceManager);
        device->setConfig(config);
        deviceManager->addDevice(device);
    }

    for (const AlarmInfo &alarm : data.alarmHistory) {
        int row = ui->alarmTable->rowCount();
        ui->alarmTable->insertRow(row);
        ui->alarmTable->setItem(row, 0, new QTableWidgetItem(QString::number(alarm.deviceId)));
        ui->alarmTable->setItem(row, 1, new QTableWidgetItem(alarm.recovered ? "恢复" : "报警"));
        ui->alarmTable->setItem(row, 2, new QTableWidgetItem(alarm.message));
        ui->alarmTable->setItem(row, 3, new QTableWidgetItem(alarm.timestamp.toString("HH:mm:ss")));
        if (auto *typeItem = ui->alarmTable->item(row, 1))
            typeItem->setForeground(alarm.recovered ? QColor(QStringLiteral("#15803D"))
                                                     : QColor(QStringLiteral("#B91C1C")));
    }

    for (const AlarmInfo &alarm : data.activeAlarms) {
        int row = ui->currentAlarmTable->rowCount();
        ui->currentAlarmTable->insertRow(row);
        ui->currentAlarmTable->setItem(row, 0, new QTableWidgetItem(QString::number(alarm.deviceId)));
        auto *typeItem = new QTableWidgetItem(alarmTypeToString(alarm.type));
        typeItem->setData(Qt::UserRole, static_cast<int>(alarm.type));
        ui->currentAlarmTable->setItem(row, 1, typeItem);
        ui->currentAlarmTable->setItem(row, 2, new QTableWidgetItem(alarm.message));
        ui->currentAlarmTable->setItem(row, 3, new QTableWidgetItem(alarm.timestamp.toString("HH:mm:ss")));
        auto *ackItem = new QTableWidgetItem(alarm.acknowledged ? QStringLiteral("已确认")
                                                                : QStringLiteral("未确认"));
        if (alarm.acknowledged)
            ackItem->setForeground(QColor(QStringLiteral("#15803D")));
        ui->currentAlarmTable->setItem(row, 4, ackItem);
    }

    if (!data.activeAlarms.isEmpty())
        m_alarmBlinkTimer->start();
    deviceManager->restoreActiveAlarms(data.activeAlarms);
    updateAlarmBlinking();

    qDebug() << "后台加载完成：设备" << data.devices.size()
             << "条，历史报警" << data.alarmHistory.size()
             << "条，当前报警" << data.activeAlarms.size() << "条";
    deviceManager->startPolling();
    if (timer && !timer->isActive())
        timer->start(1000);
}
