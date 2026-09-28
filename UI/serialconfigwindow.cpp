#include "serialconfigwindow.h"
#include "ui_serialconfigwindow.h"

#include <QMessageBox>
#include <QSerialPort>

SerialConfigWindow::SerialConfigWindow(DeviceManager *deviceManager,QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SerialConfigWindow)
    , m_deviceManager(deviceManager)
{
    ui->setupUi(this);
    setWindowFlag(Qt::Window); // 建立新窗口
    setAttribute(Qt::WA_DeleteOnClose);

    // 初始化波特率选项
    ui->baudRateComboBox->addItems({
        "9600",
        "19200",
        "38400",
        "57600",
        "115200"
    });

    ui->baudRateComboBox->setCurrentText("9600");

    // 数据位
    ui->dataBitsComboBox->addItem("5", QSerialPort::Data5);
    ui->dataBitsComboBox->addItem("6", QSerialPort::Data6);
    ui->dataBitsComboBox->addItem("7", QSerialPort::Data7);
    ui->dataBitsComboBox->addItem("8", QSerialPort::Data8);
    ui->dataBitsComboBox->setCurrentIndex(3);

    // 校验位
    ui->parityComboBox->addItem("无校验", QSerialPort::NoParity);
    ui->parityComboBox->addItem("奇校验", QSerialPort::OddParity);
    ui->parityComboBox->addItem("偶校验", QSerialPort::EvenParity);
    ui->parityComboBox->setCurrentIndex(0);

    // 停止位
    ui->stopBitsComboBox->addItem("1", QSerialPort::OneStop);
    ui->stopBitsComboBox->addItem("2", QSerialPort::TwoStop);
    ui->stopBitsComboBox->setCurrentIndex(0);

    // 初始化串口列表
    refreshSerialPorts();

    // 初始按钮状态
    ui->openSerialButton->setEnabled(true);
    ui->closeSerialButton->setEnabled(false);
    updateSerialStatus();

    // 串口状态信号
    connect(
        m_deviceManager,
        &DeviceManager::serialPortOpened,
        this,
        [this]()
        {
            updateSerialStatus();
        }
        );

    connect(
        m_deviceManager,
        &DeviceManager::serialPortClosed,
        this,
        [this]()
        {
            updateSerialStatus();
        }
        );

    connect(
        m_deviceManager,
        &DeviceManager::serialPortError,
        this,
        [this](const QString &message)
        {
            ui->serialStatusLabel->setText(
                "串口错误：" + message
                );
        }
        );
}

SerialConfigWindow::~SerialConfigWindow()
{
    delete ui;
}

void SerialConfigWindow::refreshSerialPorts()
{
    ui->serialPortComboBox->clear();

    QStringList ports = m_deviceManager->availableSerialPorts();
    ui->serialPortComboBox->addItems(ports);

    if (ports.isEmpty()) {
        ui->serialStatusLabel->setText("未检测到可用串口");
    } else {
        ui->serialStatusLabel->setText("请选择串口");
    }
}

void SerialConfigWindow::updateSerialStatus()
{
    bool isOpen = m_deviceManager->isSerialPortOpen();

    ui->serialStatusLabel->setText(
        isOpen ? "串口：已连接" : "串口：未连接"
        );

    ui->openSerialButton->setEnabled(!isOpen);
    ui->closeSerialButton->setEnabled(isOpen);

    ui->serialPortComboBox->setEnabled(!isOpen);
    ui->baudRateComboBox->setEnabled(!isOpen);
    ui->dataBitsComboBox->setEnabled(!isOpen);
    ui->parityComboBox->setEnabled(!isOpen);
    ui->stopBitsComboBox->setEnabled(!isOpen);
}

void SerialConfigWindow::on_refreshSerialButton_clicked()
{
    refreshSerialPorts();
}

void SerialConfigWindow::on_openSerialButton_clicked()
{
    // 获取选择的串口
    QString portName = ui->serialPortComboBox->currentText();

    if (portName.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择串口");
        return;
    }

    // 获取串口参数
    qint32 baudRate = ui->baudRateComboBox->currentText().toInt();

    auto dataBits = static_cast<QSerialPort::DataBits>(
        ui->dataBitsComboBox->currentData().toInt());

    auto parity = static_cast<QSerialPort::Parity>(
        ui->parityComboBox->currentData().toInt());

    auto stopBits = static_cast<QSerialPort::StopBits>(
        ui->stopBitsComboBox->currentData().toInt());

    // 打开串口
    bool success = m_deviceManager->openSerialPort(
        portName, baudRate, dataBits, parity, stopBits);

    if (!success) {
        QMessageBox::critical(this, "错误", "串口打开失败");
        return;
    }

    // 切换为串口数据模式
    m_deviceManager->setDataSource(DataSource::Serial);

    // 更新界面状态
    updateSerialStatus();
}


void SerialConfigWindow::on_closeSerialButton_clicked()
{
    // 关闭串口
    m_deviceManager->closeSerialPort();

    // 更新界面状态
    updateSerialStatus();
}
