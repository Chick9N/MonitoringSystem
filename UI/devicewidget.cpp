#include "devicewidget.h"
#include "ui_devicewidget.h"

#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QDateTimeAxis>

#include <QDateTime>
DeviceWidget::DeviceWidget(Device *device, DatabaseManager *databaseManager, QWidget *parent)
    : QWidget(parent)
    , m_databaseManager(databaseManager) // 直接复用数据库链接
    , ui(new Ui::DeviceWidget)
    , m_device(device)
{
    ui->setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose); // 不写这句, 会出现无法再次打开详情页的bug. 因为对象没有被销毁而是隐藏.
    setWindowFlag(Qt::Window); // 建立新窗口

    connect(m_device,
            &Device::dataUpdated,
            this,
            &DeviceWidget::updateWidget);

    // 初始化历史图表
    setupCharts();

    // 从 SQLite 加载历史数据
    // loadHistory();

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

        updateCharts(data);
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

void DeviceWidget::setupCharts()
{
    // 创建温度图
    m_temperatureSeries = new QLineSeries();

    QChart *temperatureChart = new QChart();

    temperatureChart->addSeries(m_temperatureSeries);
    temperatureChart->setTitle("温度历史趋势");
    temperatureChart->legend()->hide();

    temperatureChart->setMargins(
        QMargins(10, 10, 10, 10)
        );

    // 温度 X 轴：时间
    m_temperatureAxisX = new QDateTimeAxis();

    m_temperatureAxisX->setFormat("HH:mm:ss");
    m_temperatureAxisX->setTitleText("时间");
    m_temperatureAxisX->setTickCount(6);

    // 温度 Y 轴
    m_temperatureAxisY =
        new QValueAxis();

    m_temperatureAxisY->setTitleText("温度 (°C)");
    m_temperatureAxisY->setLabelFormat("%.1f");
    m_temperatureAxisY->setTickCount(5);

    temperatureChart->addAxis(
        m_temperatureAxisX,
        Qt::AlignBottom
        );

    temperatureChart->addAxis(
        m_temperatureAxisY,
        Qt::AlignLeft
        );

    m_temperatureSeries->attachAxis(
        m_temperatureAxisX
        );

    m_temperatureSeries->attachAxis(
        m_temperatureAxisY
        );

    m_temperatureChartView =
        new QChartView(temperatureChart);

    m_temperatureChartView->setRenderHint(
        QPainter::Antialiasing
        );
    // 创建电压图
    m_voltageSeries = new QLineSeries();

    QChart *voltageChart = new QChart();

    voltageChart->addSeries(m_voltageSeries);
    voltageChart->setTitle("电压历史趋势");
    voltageChart->legend()->hide();

    voltageChart->setMargins(
        QMargins(10, 10, 10, 10)
        );

    // 电压 X 轴：时间
    m_voltageAxisX = new QDateTimeAxis();

    m_voltageAxisX->setFormat("HH:mm:ss");
    m_voltageAxisX->setTitleText("时间");
    m_voltageAxisX->setTickCount(6);

    // 电压 Y 轴
    m_voltageAxisY =
        new QValueAxis();

    m_voltageAxisY->setTitleText("电压 (V)");
    m_voltageAxisY->setLabelFormat("%.2f");
    m_voltageAxisY->setTickCount(5);

    voltageChart->addAxis(
        m_voltageAxisX,
        Qt::AlignBottom
        );

    voltageChart->addAxis(
        m_voltageAxisY,
        Qt::AlignLeft
        );

    m_voltageSeries->attachAxis(
        m_voltageAxisX
        );

    m_voltageSeries->attachAxis(
        m_voltageAxisY
        );

    m_voltageChartView =
        new QChartView(voltageChart);

    m_voltageChartView->setRenderHint(
        QPainter::Antialiasing
        );

    // 把温度图放进 UI
    QLayout *temperatureLayout =
        ui->temperatureChartWidget->layout();

    temperatureLayout->setContentsMargins(0, 0, 0, 0);
    temperatureLayout->addWidget(m_temperatureChartView);

    // 把电压图放进 UI
    QLayout *voltageLayout =
        ui->voltageChartWidget->layout();

    voltageLayout->setContentsMargins(0, 0, 0, 0);
    voltageLayout->addWidget(m_voltageChartView);
}

void DeviceWidget::updateCharts(const DeviceData &data)
{
    qint64 timestamp =
        QDateTime::currentMSecsSinceEpoch();

    m_temperatureSeries->append(
        timestamp,
        data.temperature
        );

    m_voltageSeries->append(
        timestamp,
        data.voltage
        );

    constexpr int MaxPoints = 60;

    while (m_temperatureSeries->count() > MaxPoints)
        m_temperatureSeries->remove(0);

    while (m_voltageSeries->count() > MaxPoints)
        m_voltageSeries->remove(0);

    if (m_temperatureSeries->count() > 1)
    {
        qint64 minTime =
            static_cast<qint64>(
                m_temperatureSeries->at(0).x()
                );

        qint64 maxTime =
            static_cast<qint64>(
                m_temperatureSeries->at(
                                       m_temperatureSeries->count() - 1
                                       ).x()
                );

        m_temperatureAxisX->setRange(
            QDateTime::fromMSecsSinceEpoch(minTime),
            QDateTime::fromMSecsSinceEpoch(maxTime)
            );

        m_voltageAxisX->setRange(
            QDateTime::fromMSecsSinceEpoch(minTime),
            QDateTime::fromMSecsSinceEpoch(maxTime)
            );
    }

    m_temperatureAxisY->setRange(15, 65);
    m_voltageAxisY->setRange(210, 230);
}

void DeviceWidget::loadHistory()
{
    QList<DeviceHistory> history =
        m_databaseManager->queryDeviceHistory(
            m_device->id()
            );

    qDebug() << "设备" << m_device->id()
             << "历史数据数量：" << history.size();

    if (history.isEmpty())
        return;

    m_temperatureSeries->clear();
    m_voltageSeries->clear();

    for (const DeviceHistory &item : history)
    {
        qint64 timestamp =
            item.timestamp.toMSecsSinceEpoch();

        m_temperatureSeries->append(
            timestamp,
            item.data.temperature
            );

        m_voltageSeries->append(
            timestamp,
            item.data.voltage
            );
    }

    // X轴时间范围
    qint64 startTime =
        history.first().timestamp.toMSecsSinceEpoch();

    qint64 endTime =
        history.last().timestamp.toMSecsSinceEpoch();

    // 如果只有一个时间点，避免坐标轴范围为0
    if (startTime == endTime)
    {
        startTime -= 1000;
        endTime += 1000;
    }

    m_temperatureAxisX->setRange(
        QDateTime::fromMSecsSinceEpoch(startTime),
        QDateTime::fromMSecsSinceEpoch(endTime)
        );

    m_voltageAxisX->setRange(
        QDateTime::fromMSecsSinceEpoch(startTime),
        QDateTime::fromMSecsSinceEpoch(endTime)
        );

    // Y轴范围
    m_temperatureAxisY->setRange(15, 65);
    m_voltageAxisY->setRange(210, 230);
}
