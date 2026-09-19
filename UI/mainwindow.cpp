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

    connect(deviceManager,
            &DeviceManager::deviceDataUpdated,
            this,
            [this](int deviceId, const DeviceData &data){
                databaseManager->insertDeviceData(deviceId,data);
            }
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
