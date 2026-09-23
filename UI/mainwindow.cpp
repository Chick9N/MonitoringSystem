#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "../UI/devicewidget.h"
#include <qtablewidget.h>
#include "minichartwidget.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , deviceManager(new DeviceManager(this))
    , databaseManager(new DatabaseManager(this))
    , timer(new QTimer(this))
{
    ui->setupUi(this);
    ui->deviceTable->setRowCount(3);
    ui->deviceTable->setColumnCount(6);
    ui->deviceTable->horizontalHeader()->setSectionResizeMode(
        4, QHeaderView::Stretch);
    ui->deviceTable->setHorizontalHeaderLabels({"设备ID","状态","温度","电压","温度走势图","电压走势图"});
    ui->deviceTable->horizontalHeader()
        ->setSectionResizeMode(
            0,
            QHeaderView::ResizeToContents
            );

    ui->deviceTable->horizontalHeader()
        ->setSectionResizeMode(
            1,
            QHeaderView::ResizeToContents
            );

    ui->deviceTable->horizontalHeader()
        ->setSectionResizeMode(
            2,
            QHeaderView::ResizeToContents
            );

    ui->deviceTable->horizontalHeader()
        ->setSectionResizeMode(
            3,
            QHeaderView::ResizeToContents
            );

    ui->deviceTable->horizontalHeader()
        ->setSectionResizeMode(
            4,
            QHeaderView::Stretch
            );

    ui->deviceTable->horizontalHeader()
        ->setSectionResizeMode(
            5,
            QHeaderView::Stretch
            );

    Device *device1 = new Device(1,this);
    Device *device2 = new Device(2,this);
    Device *device3 = new Device(3,this);
    // 一定使用add方法添加设备 add方法内部构造信号槽
    deviceManager->addDevice(device1);
    deviceManager->addDevice(device2);
    deviceManager->addDevice(device3);
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

    timer->start(1000);

    // 数据库
    databaseManager->openDatabase();
    databaseManager->createTables();

    loadAlarmHistory();
    loadCurrentAlarms();

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

}

void MainWindow::updateDeviceUI(int deviceId, const DeviceData &data)
{
    int row = deviceId - 1;

    ui->deviceTable->setRowHeight(row, 120);

    ui->deviceTable->setItem(
        row, 0,
        new QTableWidgetItem(QString::number(deviceId))
        );

    ui->deviceTable->setItem(
        row, 1,
        new QTableWidgetItem(
            data.isOnline ? "在线" : "离线"
            )
        );

    MiniChartWidget *temperatureChart =
        qobject_cast<MiniChartWidget*>(
            ui->deviceTable->cellWidget(row, 4)
            );

    if (!temperatureChart)
    {
        temperatureChart =
            new MiniChartWidget(
                ChartType::Temperature
                );

        ui->deviceTable->setCellWidget(
            row,
            4,
            temperatureChart
            );
    }

    MiniChartWidget *voltageChart =
        qobject_cast<MiniChartWidget*>(
            ui->deviceTable->cellWidget(row, 5)
            );

    if (!voltageChart)
    {
        voltageChart =
            new MiniChartWidget(
                ChartType::Voltage
                );

        ui->deviceTable->setCellWidget(
            row,
            5,
            voltageChart
            );
    }

    if (data.isOnline)
    {
        ui->deviceTable->setItem(
            row, 2,
            new QTableWidgetItem(
                QString::number(data.temperature, 'f', 1)
                )
            );

        ui->deviceTable->setItem(
            row, 3,
            new QTableWidgetItem(
                QString::number(data.voltage, 'f', 1)
                )
            );

        // 保存历史数据
        m_temperatureHistory[deviceId].append(
            data.temperature
            );

        m_voltageHistory[deviceId].append(
            data.voltage
            );

        // 限制长度
        if (m_temperatureHistory[deviceId].size()
            > MaxHistoryPoints)
        {
            m_temperatureHistory[deviceId].removeFirst();
        }

        if (m_voltageHistory[deviceId].size()
            > MaxHistoryPoints)
        {
            m_voltageHistory[deviceId].removeFirst();
        }

        // 更新小图
        temperatureChart->setData(
            m_temperatureHistory[deviceId]
            );

        voltageChart->setData(
            m_voltageHistory[deviceId]
            );
    }
    else
    {
        ui->deviceTable->setItem(
            row, 2,
            new QTableWidgetItem("--")
            );

        ui->deviceTable->setItem(
            row, 3,
            new QTableWidgetItem("--")
            );
    }
}


MainWindow::~MainWindow()
{
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

    int deviceId = row + 1;

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
    QList<AlarmInfo> alarms =
        databaseManager->queryAlarmHistory();

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

