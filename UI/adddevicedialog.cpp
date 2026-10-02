#include "adddevicedialog.h"
#include "ui_adddevicedialog.h"
#include <QMessageBox>
#include <QDebug>

AddDeviceDialog::AddDeviceDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AddDeviceDialog)
{
    ui->setupUi(this);
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
    // 获取设备名称
    QString deviceName = ui->nameEdit->text().trimmed();

    // 检查设备名称
    if (deviceName.isEmpty())
    {
        QMessageBox::warning(
            this,
            "输入错误",
            "设备名称不能为空！"
            );
        return;
    }

    // 配置数据由外部通过 getDeviceConfig() 获取

    accept();
}


void AddDeviceDialog::on_buttonBox_rejected()
{
    reject();
}

DeviceConfig AddDeviceDialog::getDeviceConfig() const
{
    DeviceConfig config;

    // 基本信息
    config.deviceId = ui->deviceIdSpinBox->value();
    config.deviceName = ui->nameEdit->text().trimmed();
    config.protocolType = static_cast<ProtocolType>(
        ui->protocolComboBox->currentIndex()
        );

    // 根据协议确定数据来源
    switch (config.protocolType)
    {
    case ProtocolType::Custom:
    case ProtocolType::ModbusRTU:
        config.dataSource = DataSource::Serial;
        break;

    case ProtocolType::ModbusTCP:
        config.dataSource = DataSource::TCP;
        break;
    }

    // 自定义串口配置
    config.serialPort = ui->serialPortComboBox->currentText();
    config.baudRate = ui->baudRateComboBox->currentText().toInt();
    config.dataBits = ui->dataBitsComboBox->currentText().toInt();
    config.stopBits = ui->stopBitsComboBox->currentText().toInt();
    config.parity = ui->parityComboBox->currentText();

    // Modbus RTU 配置
    config.rtuSlaveId = ui->rtuSlaveIdSpinBox->value();
    config.rtuStartAddress = ui->rtuStartAddressSpinBox->value();
    config.rtuQuantity = ui->rtuQuantitySpinBox->value();

    // Modbus TCP 配置
    config.tcpIp = ui->tcpIpEdit->text().trimmed();
    config.tcpPort = ui->tcpPortSpinBox->value();
    config.tcpUnitId = ui->tcpUnitIdSpinBox->value();
    config.tcpStartAddress = ui->tcpStartAddressSpinBox->value();
    config.tcpQuantity = ui->tcpQuantitySpinBox->value();

    return config;
}
