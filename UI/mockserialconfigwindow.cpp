#include "mockserialconfigwindow.h"
#include "ui_mockserialconfigwindow.h"

#include "../Communication/mockserialdevice.h"

#include <QMessageBox>
#include <QSerialPort>
#include <QSerialPortInfo>

MockSerialConfigWindow::MockSerialConfigWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MockSerialConfigWindow)
    , m_mockDevice(new MockSerialDevice(this))
{
    ui->setupUi(this);

    setWindowFlag(Qt::Window);
    setAttribute(Qt::WA_DeleteOnClose);

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

    // 模拟自定义串口协议目前固定使用 8N1
    ui->dataBitsComboBox->addItem(
        "5",
        QSerialPort::Data5
        );

    ui->dataBitsComboBox->addItem(
        "6",
        QSerialPort::Data6
        );

    ui->dataBitsComboBox->addItem(
        "7",
        QSerialPort::Data7
        );

    ui->dataBitsComboBox->addItem(
        "8",
        QSerialPort::Data8
        );

    ui->dataBitsComboBox->setCurrentIndex(3);

    // 校验位
    ui->parityComboBox->addItem(
        "无校验",
        QSerialPort::NoParity
        );

    ui->parityComboBox->addItem(
        "奇校验",
        QSerialPort::OddParity
        );

    ui->parityComboBox->addItem(
        "偶校验",
        QSerialPort::EvenParity
        );

    ui->parityComboBox->setCurrentIndex(0);

    // 停止位
    ui->stopBitsComboBox->addItem(
        "1",
        QSerialPort::OneStop
        );

    ui->stopBitsComboBox->addItem(
        "2",
        QSerialPort::TwoStop
        );

    ui->stopBitsComboBox->setCurrentIndex(0);

    // 当前 MockSerialDevice::start()
    // 只接收 portName 和 baudRate，并固定使用 8N1。
    // 因此这里将其他参数固定，避免界面配置与实际行为不一致。
    ui->dataBitsComboBox->setEnabled(false);
    ui->parityComboBox->setEnabled(false);
    ui->stopBitsComboBox->setEnabled(false);

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

    delete ui;
}

void MockSerialConfigWindow::refreshSerialPorts()
{
    ui->serialPortComboBox->clear();

    const auto ports =
        QSerialPortInfo::availablePorts();

    for (const QSerialPortInfo &info : ports)
    {
        ui->serialPortComboBox->addItem(
            info.portName()
            );
    }

    if (ports.isEmpty())
    {
        ui->serialStatusLabel->setText(
            "未检测到可用串口"
            );
    }
    else
    {
        ui->serialStatusLabel->setText(
            "请选择模拟设备串口"
            );
    }
}

void MockSerialConfigWindow::updateSerialStatus()
{
    const bool isOpen =
        m_mockDevice &&
        m_mockDevice->isRunning();

    ui->serialStatusLabel->setText(
        isOpen
            ? "模拟设备：已启动"
            : "模拟设备：未启动"
        );

    ui->openSerialButton->setEnabled(!isOpen);
    ui->closeSerialButton->setEnabled(isOpen);

    ui->serialPortComboBox->setEnabled(!isOpen);
    ui->baudRateComboBox->setEnabled(!isOpen);
    ui->refreshSerialButton->setEnabled(!isOpen);
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
    if (!m_mockDevice)
        return;

    m_mockDevice->stop();

    updateSerialStatus();
}
