#include "adddevicedialog.h"
#include "ui_adddevicedialog.h"
#include "../Communication/serialport.h"

#include <QMessageBox>
#include <QDebug>
#include <QHBoxLayout>
#include <QHostAddress>


AddDeviceDialog::AddDeviceDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AddDeviceDialog)
{
    ui->setupUi(this);
    ui->serialPortComboBox->addItems(SerialPort::availablePorts());
    ui->rtuPortComboBox->addItems(SerialPort::availablePorts());
}

AddDeviceDialog::~AddDeviceDialog()
{
    delete ui;
}

void AddDeviceDialog::on_protocolComboBox_currentIndexChanged(int index)
{
    ui->configStackedWidget->setCurrentIndex(index);
}


void AddDeviceDialog::on_buttonBox_accepted()
{
    accept();
}


void AddDeviceDialog::on_buttonBox_rejected()
{
    reject();
}

void AddDeviceDialog::accept()
{
    // 检查设备名称
    QString name = ui->nameEdit->text().trimmed();

    if (name.isEmpty()) {
        QMessageBox::warning(
            this,
            "输入错误",
            "设备名称不能为空，请填写设备名称！"
            );

        return; // 不关闭表单，保留已填写的内容
    }

    if (static_cast<ProtocolType>(
        ui->protocolComboBox->currentIndex())
        == ProtocolType::ModbusTCP) {
        if (!ui->ipWidget->isValid()) {
            QMessageBox::warning(
                this,
                "输入错误",
                "请输入完整且有效的 IPv4 地址！"
                );
            return;
        }
    }

    // 校验通过，关闭表单并返回 Accepted
    QDialog::accept();
}

DeviceConfig AddDeviceDialog::getDeviceConfig() const
{
    DeviceConfig config;

    // ==============================
    // 基本信息
    // ==============================
    config.deviceId =
        ui->deviceIdSpinBox->value();

    config.deviceName =
        ui->nameEdit->text().trimmed();

    config.protocolType =
        static_cast<ProtocolType>(
            ui->protocolComboBox->currentIndex()
            );


    // ==============================
    // 根据协议读取对应配置
    // ==============================
    switch (config.protocolType)
    {
    // ==========================================
    // 自定义串口协议
    // ==========================================
    case ProtocolType::Custom:
    {
        config.dataSource =
            DataSource::Serial;

        config.serialPort =
            ui->serialPortComboBox->currentText();

        config.baudRate =
            ui->baudRateComboBox->currentText().toInt();

        config.dataBits =
            ui->dataBitsComboBox->currentText().toInt();

        config.parity =
            ui->parityComboBox->currentText();

        config.stopBits =
            ui->stopBitsComboBox->currentText().toInt();

        break;
    }


        // ==========================================
        // Modbus RTU
        // ==========================================
    case ProtocolType::ModbusRTU:
    {
        config.dataSource =
            DataSource::Serial;

        // 串口基本参数
        config.serialPort =
            ui->rtuPortComboBox->currentText();

        config.baudRate =
            ui->baudRateComboBox->currentText().toInt();

        // 当前项目 RTU 固定使用 8N1
        config.dataBits = 8;
        config.parity = "None";
        config.stopBits = 1;

        // Modbus RTU 参数
        config.rtuSlaveId =
            ui->rtuSlaveIdSpinBox->value();

        config.rtuStartAddress =
            ui->rtuStartAddressSpinBox->value();

        config.rtuQuantity =
            ui->rtuQuantitySpinBox->value();

        break;
    }


        // ==========================================
        // Modbus TCP
        // ==========================================
    case ProtocolType::ModbusTCP:
    {
        config.dataSource =
            DataSource::TCP;

        config.tcpIp =
            ui->ipWidget->address();

        config.tcpPort =
            ui->tcpPortSpinBox->value();

        config.tcpUnitId =
            ui->tcpUnitIdSpinBox->value();

        config.tcpStartAddress =
            ui->tcpStartAddressSpinBox->value();

        config.tcpQuantity =
            ui->tcpQuantitySpinBox->value();

        break;
    }
    }

    return config;
}

void AddDeviceDialog::on_testConnectionBtn_clicked()
{

    QString ip = ui->ipWidget->address();

    quint16 port =
        ui->tcpPortSpinBox->value();


    // IP校验
    QHostAddress address;

    if(!address.setAddress(ip))
    {
        QMessageBox::warning(
            this,
            "输入错误",
            "IP地址格式错误"
            );

        return;
    }


    // 端口校验

    if(port <=0 || port >65535)
    {
        QMessageBox::warning(
            this,
            "输入错误",
            "端口范围错误"
            );

        return;
    }

    ui->testConnectionBtn->setEnabled(false);

    emit testTCPConnection(
        ip,
        port
        );
}

void AddDeviceDialog::onTCPTestResult(
    bool success,
    const QString &message
    )
{
    if(success)
    {
        ui->connectionStatusLabel->setText("连接成功");
    }
    else
    {
        ui->connectionStatusLabel->setText("连接失败"+message);
    }
}
