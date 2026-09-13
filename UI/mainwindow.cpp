#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <qtablewidget.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , deviceManager(new DeviceManager(this))
    , timer(new QTimer(this))
{
    ui->setupUi(this);
    ui->deviceTable->setRowCount(3);
    ui->deviceTable->setColumnCount(4);
    ui->deviceTable->setHorizontalHeaderLabels({"设备ID","状态","温度","电压"});

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
}

void MainWindow::updateDeviceUI(int deviceId,const DeviceData &data){
    int row = deviceId-1;
    ui->deviceTable->setItem(
        row,0,
        new QTableWidgetItem(
            QString::number(deviceId))
        );

    ui->deviceTable->setItem(
        row,1,
        new QTableWidgetItem(
            data.isOnline ? "在线":"离线")
        );

    if(data.isOnline){
        ui->deviceTable->setItem(
            row,2,
            new QTableWidgetItem(
                QString::number(data.temperature,'f',1))
            );

        ui->deviceTable->setItem(
            row,3,
            new QTableWidgetItem(
                QString::number(data.voltage,'f',1))
            );
    }else{
        ui->deviceTable->setItem(
            row,2,
            new QTableWidgetItem("--")
            );

        ui->deviceTable->setItem(
            row,3,
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

