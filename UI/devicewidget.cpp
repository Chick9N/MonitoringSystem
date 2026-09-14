#include "devicewidget.h"
#include "ui_devicewidget.h"

DeviceWidget::DeviceWidget(Device *device, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::DeviceWidget)
    , m_device(device)
{
    ui->setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose); // 不写这句, 会出现无法再次打开详情页的bug. 因为对象没有被销毁而是隐藏.

    connect(m_device,
            &Device::dataUpdated,
            this,
            &DeviceWidget::updateWidget);

    updateWidget(m_device->data());
}

DeviceWidget::~DeviceWidget()
{
    delete ui;
}

void DeviceWidget::updateWidget(const DeviceData &data){
    ui->startBtn->setEnabled(!data.isOnline);
    ui->stopBtn->setEnabled(data.isOnline);
    ui->statusLabel->setText(data.isOnline ? "状态：在线":"状态：离线");
    if(data.isOnline){
        ui->deviceNameLabel->setText(
            QString("设备编号：%1").arg(m_device->id())
            );

        ui->temperatureLabel->setText(
            QString("温度：%1 ℃").arg(data.temperature,0,'f',1)
        );

        ui->voltageLabel->setText(
            QString("电压：%1 V").arg(data.voltage,0,'f',1)
            );
    }else{
        ui->deviceNameLabel->setText("设备编号：--");
        ui->temperatureLabel->setText("温度：--");
        ui->voltageLabel->setText("电压：--");
    }
}

void DeviceWidget::on_startBtn_clicked()
{
    m_device->start();
}


void DeviceWidget::on_stopBtn_clicked()
{
    m_device->stop();
}

