#include "mockserialconfigwindow.h"
#include "ui_mockserialconfigwindow.h"

#include "../Communication/mockserialdevice.h"
#include "../Communication/mockmodbusrtudevice.h"

#include <QMessageBox>
#include <QSerialPortInfo>
#include <QShowEvent>

MockSerialConfigWindow::MockSerialConfigWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MockSerialConfigWindow)
    , m_mockDevice(new MockSerialDevice(this))
    , m_modbusDevice(new MockModbusRTUDevice(this))
{
    ui->setupUi(this);

    setWindowFlag(Qt::Window);

    setWindowTitle("模拟串口设备配置");


    // 初始化波特率
    ui->baudRateComboBox->addItems({
        "9600",
        "19200",
        "38400",
        "57600",
        "115200"
    });

    ui->baudRateComboBox->setCurrentText("9600");

    // 初始化串口列表
    refreshSerialPorts();

    // 初始状态
    ui->openSerialButton->setEnabled(true);
    ui->closeSerialButton->setEnabled(false);

    updateSerialStatus();

    // MockSerialDevice 启动成功
    connect(
        m_mockDevice,
        &MockSerialDevice::started,
        this,
        [this]()
        {
            updateSerialStatus();
        }
        );

    connect(
        m_modbusDevice,
        &MockModbusRTUDevice::started,
        this,
        [this]() { updateSerialStatus(); }
        );

    connect(
        m_modbusDevice,
        &MockModbusRTUDevice::stopped,
        this,
        [this]() { updateSerialStatus(); }
        );

    // MockSerialDevice 停止
    connect(
        m_mockDevice,
        &MockSerialDevice::stopped,
        this,
        [this]()
        {
            updateSerialStatus();
        }
        );
}

MockSerialConfigWindow::~MockSerialConfigWindow()
{
    if (m_mockDevice)
    {
        m_mockDevice->stop();
    }

    if (m_modbusDevice)
    {
        m_modbusDevice->stop();
    }

    delete ui;
}

void MockSerialConfigWindow::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    refreshSerialPorts();
    updateSerialStatus();
}

void MockSerialConfigWindow::refreshSerialPorts()
{
    const QString selectedPort = ui->serialPortComboBox->currentText();
    ui->serialPortComboBox->clear();

    const auto ports =
        QSerialPortInfo::availablePorts();

    for (const QSerialPortInfo &info : ports)
    {
        const int index = ui->serialPortComboBox->count();
        ui->serialPortComboBox->addItem(info.portName());
        ui->serialPortComboBox->setItemData(index, info.description(), Qt::ToolTipRole);
    }

    const int selectedIndex = ui->serialPortComboBox->findText(selectedPort);
    if (selectedIndex >= 0)
        ui->serialPortComboBox->setCurrentIndex(selectedIndex);

    if (ports.isEmpty())
    {
        ui->serialStatusLabel->setText("未检测到可用串口");
    }
    updateSerialStatus();
}

void MockSerialConfigWindow::updateSerialStatus()
{
    const bool isOpen = (m_mockDevice && m_mockDevice->isRunning())
        || (m_modbusDevice && m_modbusDevice->isRunning());
    const bool modbusRunning = m_modbusDevice && m_modbusDevice->isRunning();

    if (isOpen) {
        const QString mode = modbusRunning ? "Modbus RTU模拟器" : "自定义协议模拟器";
        ui->serialStatusLabel->setText(
            QString("%1：%2 已打开（%3 波特）")
                .arg(mode,
                     modbusRunning ? m_modbusDevice->portName() : m_mockDevice->portName())
                .arg(modbusRunning ? m_modbusDevice->baudRate() : m_mockDevice->baudRate()));
    } else {
        ui->serialStatusLabel->setText("模拟器串口：未打开");
    }

    ui->openSerialButton->setEnabled(!isOpen);
    ui->closeSerialButton->setEnabled(isOpen);

    ui->serialPortComboBox->setEnabled(!isOpen);
    ui->baudRateComboBox->setEnabled(!isOpen);
    ui->refreshSerialButton->setEnabled(!isOpen);
    ui->modbusModeCheckBox->setEnabled(!isOpen);
}

void MockSerialConfigWindow::on_refreshSerialButton_clicked()
{
    refreshSerialPorts();
}

void MockSerialConfigWindow::on_openSerialButton_clicked()
{
    const QString portName =
        ui->serialPortComboBox->currentText();

    if (portName.isEmpty())
    {
        QMessageBox::warning(
            this,
            "提示",
            "请先选择模拟设备串口"
            );

        return;
    }

    const qint32 baudRate =
        ui->baudRateComboBox->currentText().toInt();

    // ==============================
    // Modbus RTU 模式
    // ==============================
    if (m_modbusMode)
    {
        if (m_modbusDevice->isRunning())
        {
            QMessageBox::information(
                this,
                "提示",
                "Modbus RTU 模拟设备已经启动"
                );

            return;
        }

        const bool success =
            m_modbusDevice->start(
                portName,
                baudRate
                );

        if (!success)
        {
            QMessageBox::critical(
                this,
                "错误",
                "Modbus RTU 模拟设备串口启动失败"
                );

            updateSerialStatus();

            return;
        }

        updateSerialStatus();
        return;
    }

    // ==============================
    // 自定义串口模式
    // ==============================
    if (m_mockDevice->isRunning())
    {
        QMessageBox::information(
            this,
            "提示",
            "模拟设备已经启动"
            );

        return;
    }

    const bool success =
        m_mockDevice->start(
            portName,
            baudRate
            );

    if (!success)
    {
        QMessageBox::critical(
            this,
            "错误",
            "模拟设备串口启动失败"
            );

        updateSerialStatus();

        return;
    }

    updateSerialStatus();
}

void MockSerialConfigWindow::on_closeSerialButton_clicked()
{
    if (m_modbusMode)
    {
        if (m_modbusDevice)
            m_modbusDevice->stop();
    }
    else if (m_mockDevice)
    {
        m_mockDevice->stop();
    }

    updateSerialStatus();
}

void MockSerialConfigWindow::on_modbusModeCheckBox_stateChanged(int arg1)
{
    m_modbusMode = (arg1 == Qt::Checked);

    qDebug()
        << "模拟串口模式:"
        << (m_modbusMode ? "Modbus RTU" : "自定义串口");
}
