#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "../UI/devicewidget.h"
#include <qtablewidget.h>
#include "minichartwidget.h"
#include "mockserialconfigwindow.h"
#include <QPushButton>
#include "adddevicedialog.h"
#include <QMessageBox>
#include <QtConcurrent/QtConcurrentRun>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QThread>
#include <algorithm>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , databaseManager(new DatabaseManager(this))
    , timer(new QTimer(this))
    , m_mockServer(new MockModbusTCPServer(this))
{
    deviceManager= new DeviceManager(databaseManager, this);

    ui->setupUi(this);
    ui->deviceTable->setColumnCount(9);
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

    // 设备添加 -> MainWindow 表格
    connect(
        deviceManager,
        &DeviceManager::deviceAdded,
        this,
        &MainWindow::addDeviceRow
        );

    // 启动时的数据库查询放入后台线程；该线程使用独立 SQLite 连接。
    m_startupWatcher = new QFutureWatcher<StartupData>(this);
    connect(m_startupWatcher, &QFutureWatcher<StartupData>::finished,
            this, [this]() {
        applyStartupData(m_startupWatcher->result());
    });

    const QString databasePath = databaseManager->databasePath();
    m_startupWatcher->setFuture(QtConcurrent::run([databasePath]() {
        StartupData result;
        const QString connectionName =
            QStringLiteral("startup_read_%1").arg(reinterpret_cast<quintptr>(QThread::currentThreadId()));
        {
            QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
            db.setDatabaseName(databasePath);
            if (!db.open()) {
                qWarning() << "后台打开数据库失败:" << db.lastError();
            } else {
                {
                QSqlQuery query(db);
                if (query.exec("SELECT device_id, device_name, data_source, protocol_type, serial_port, baud_rate, data_bits, stop_bits, parity, rtu_slave_id, rtu_start_address, rtu_quantity, tcp_ip, tcp_port, tcp_unit_id, tcp_start_address, tcp_quantity FROM device ORDER BY device_id")) {
                    while (query.next()) {
                        DeviceConfig c;
                        c.deviceId = query.value(0).toInt(); c.deviceName = query.value(1).toString();
                        c.dataSource = static_cast<DataSource>(query.value(2).toInt());
                        c.protocolType = static_cast<ProtocolType>(query.value(3).toInt());
                        c.serialPort = query.value(4).toString(); c.baudRate = query.value(5).toInt();
                        c.dataBits = query.value(6).toInt(); c.stopBits = query.value(7).toInt(); c.parity = query.value(8).toString();
                        c.rtuSlaveId = query.value(9).toInt(); c.rtuStartAddress = query.value(10).toInt(); c.rtuQuantity = query.value(11).toInt();
                        c.tcpIp = query.value(12).toString(); c.tcpPort = query.value(13).toInt(); c.tcpUnitId = query.value(14).toInt();
                        c.tcpStartAddress = query.value(15).toInt(); c.tcpQuantity = query.value(16).toInt();
                        result.devices.append(c);
                    }
                } else qWarning() << "后台查询设备失败:" << query.lastError();

                auto readAlarms = [&db](const QString &sql, int limit) {
                    QList<AlarmInfo> alarms;
                    QSqlQuery aq(db);
                    aq.prepare(sql);
                    if (sql.contains(":limit")) aq.bindValue(":limit", limit);
                    if (!aq.exec()) { qWarning() << "后台查询报警失败:" << aq.lastError(); return alarms; }
                    while (aq.next()) {
                        AlarmInfo a;
                        a.deviceId = aq.value(0).toInt(); a.type = static_cast<AlarmType>(aq.value(1).toInt());
                        a.message = aq.value(2).toString(); a.recovered = aq.value(3).toInt() != 0;
                        a.timestamp = QDateTime::fromString(aq.value(4).toString(), "yyyy-MM-dd HH:mm:ss");
                        alarms.append(a);
                    }
                    return alarms;
                };
                result.alarmHistory = readAlarms(
                    "SELECT device_id, alarm_type, message, recovered, timestamp FROM alarm_history ORDER BY id DESC LIMIT :limit", 500);
                std::reverse(result.alarmHistory.begin(), result.alarmHistory.end());
                result.activeAlarms = readAlarms(R"(
                    SELECT a.device_id, a.alarm_type, a.message, a.recovered, a.timestamp
                    FROM alarm_history a
                    WHERE a.recovered = 0 AND a.id = (
                        SELECT MAX(b.id) FROM alarm_history b
                        WHERE b.device_id = a.device_id AND b.alarm_type = a.alarm_type
                    ) ORDER BY a.id ASC
                )", 0);
                }
            }
            db.close();
        }
        QSqlDatabase::removeDatabase(connectionName);
        return result;
    }));

    connect(deviceManager,
            &DeviceManager::deviceDataUpdated,
            this,
            [this](int deviceId, const DeviceData &data){
                databaseManager->insertDeviceData(deviceId,data);
            }
            );

    connect(
        deviceManager,
        &DeviceManager::alarmTriggered,
        this,
        &MainWindow::handleAlarm
        );

    // 服务器
    //if (!m_mockServer->start(1502)) {
    //    qWarning() << "模拟服务器启动失败";
    //}

}

void MainWindow::removeDeviceRow(int deviceId)
{

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
    ui->deviceTable->setItem(
        row, 3,
        new QTableWidgetItem(data.isOnline ? "在线" : "离线")
        );

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
        ui->deviceTable->setItem(
            row, 4,
            new QTableWidgetItem(
                QString::number(data.temperature, 'f', 1)
                )
            );

        // 电压
        ui->deviceTable->setItem(
            row, 5,
            new QTableWidgetItem(
                QString::number(data.voltage, 'f', 1)
                )
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
    if (m_startupWatcher && m_startupWatcher->isRunning())
        m_startupWatcher->waitForFinished();
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
    databaseManager->insertAlarm(alarm);

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
}

int MainWindow::findCurrentAlarm(
    int deviceId,
    AlarmType type) const
{
    for (int row = 0;
         row < ui->currentAlarmTable->rowCount();
         ++row)
    {
        int id = ui->currentAlarmTable
                     ->item(row, 0)
                     ->text()
                     .toInt();

        AlarmType currentType =
            static_cast<AlarmType>(
                ui->currentAlarmTable
                    ->item(row, 1)
                    ->data(Qt::UserRole)
                    .toInt()
                );

        if (id == deviceId && currentType == type)
            return row;
    }

    return -1;
}

void MainWindow::loadAlarmHistory()
{
    // Retained as an explicit reload entry point; startup uses the async loader.
    QList<AlarmInfo> alarms = databaseManager->queryAlarmHistory(500);

    for (const AlarmInfo &alarm : alarms)
    {
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

        ui->alarmTable->setItem(
            row,
            2,
            new QTableWidgetItem(
                alarm.message
                )
            );

        ui->alarmTable->setItem(
            row,
            3,
            new QTableWidgetItem(
                alarm.timestamp.toString("HH:mm:ss")
                )
            );
    }
}

void MainWindow::loadCurrentAlarms()
{
    QList<AlarmInfo> alarms =
        databaseManager->queryActiveAlarms();

    for (const AlarmInfo &alarm : alarms)
    {
        int row =
            ui->currentAlarmTable->rowCount();

        ui->currentAlarmTable->insertRow(row);

        ui->currentAlarmTable->setItem(
            row,
            0,
            new QTableWidgetItem(
                QString::number(alarm.deviceId)
                )
            );

        auto *typeItem =
            new QTableWidgetItem(
                alarmTypeToString(alarm.type)
                );

        typeItem->setData(
            Qt::UserRole,
            static_cast<int>(alarm.type)
            );

        ui->currentAlarmTable->setItem(
            row,
            1,
            typeItem
            );

        ui->currentAlarmTable->setItem(
            row,
            2,
            new QTableWidgetItem(
                alarm.message
                )
            );

        ui->currentAlarmTable->setItem(
            row,
            3,
            new QTableWidgetItem(
                alarm.timestamp.toString("HH:mm:ss")
                )
            );
    }
}

void MainWindow::updateAlarmStatistics()
{
    ui->alarmCountLabel->setText(
        QString("报警事件：%1")
            .arg(ui->alarmTable->rowCount())
        );

    ui->currentAlarmCountLabel->setText(
        QString("当前报警：%1")
            .arg(ui->currentAlarmTable->rowCount())
        );
}


void MainWindow::on_mockSerialConfigBtn_clicked()
{
    MockSerialConfigWindow *window =
        new MockSerialConfigWindow(this);

    window->show();
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

    // 当前数据来源仍然是 DeviceManager 的全局配置
    switch (deviceManager->dataSource())
    {
    case DataSource::Simulation:
        sourceName = "模拟数据";
        break;

    case DataSource::Serial:
        sourceName = "真实设备";
        break;

    case DataSource::TCP:
        sourceName = "TCP";
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

    qDebug() << "MainWindow 新增设备行:" << deviceId;
}

void MainWindow::on_testBtn_clicked()
{

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

    if(!m_tcpServerRunning)
    {

        if(m_mockServer->start(1502))
        {
            m_tcpServerRunning=true;


            ui->tcpServerBtn
                ->setText("关闭TCP服务器");


            ui->tcpStatusLabel
                ->setText(
                    "TCP服务器运行中"
                    );


            qDebug()
                <<"启动Modbus TCP服务器";

        }

    }
    else
    {

        m_mockServer->stop();


        m_tcpServerRunning=false;


        ui->tcpServerBtn
            ->setText("启动TCP服务器");


        ui->tcpStatusLabel
            ->setText(
                "TCP服务器已关闭"
                );


        qDebug()
            <<"关闭Modbus TCP服务器";

    }

}

void MainWindow::loadDevicesFromDatabase()
{
    const QList<DeviceConfig> configs =
        databaseManager->queryDevices();

    for (const DeviceConfig &config : configs)
    {
        Device *device =
            new Device(
                config.deviceId,
                deviceManager
                );

        device->setConfig(config);

        deviceManager->addDevice(device);
    }

    qDebug()
        << "从数据库恢复设备数量:"
        << configs.size();
}

void MainWindow::applyStartupData(const StartupData &data)
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
    }

    qDebug() << "后台加载完成：设备" << data.devices.size()
             << "条，历史报警" << data.alarmHistory.size()
             << "条，当前报警" << data.activeAlarms.size() << "条";
    deviceManager->startPolling();
    if (timer && !timer->isActive())
        timer->start(1000);
}
