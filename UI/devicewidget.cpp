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
    setupHistoryCharts();

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
    temperatureChart->setTitle("实时温度");
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
    voltageChart->setTitle("实时电压");
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


void DeviceWidget::on_queryHistoryButton_clicked()
{
    QDateTime endTime = QDateTime::currentDateTime();
    QDateTime startTime;

    switch (ui->historyRangeComboBox->currentIndex())
    {
    case 0:
        startTime = endTime.addSecs(-60);
        break;

    case 1:
        startTime = endTime.addSecs(-10 * 60);
        break;

    case 2:
        startTime = endTime.addSecs(-60 * 60);
        break;

    case 3:
        startTime = endTime.addSecs(-24 * 60 * 60);
        break;

    default:
        return;
    }

    QList<DeviceHistory> history =
        m_databaseManager->queryDeviceHistory(
            m_device->id(),
            startTime,
            endTime
            );

    qDebug() << "查询到历史数据：" << history.size();

    QList<DeviceHistory> sampledHistory =
        sampleHistory(history, 300);
    updateHistoryCharts(sampledHistory);
}

QList<DeviceHistory> DeviceWidget::sampleHistory(
    const QList<DeviceHistory> &history,
    int maxPoints)
{
    if (history.size() <= maxPoints)
        return history;

    QList<DeviceHistory> result;

    double step =
        static_cast<double>(history.size() - 1)
        / (maxPoints - 1);

    for (int i = 0; i < maxPoints; ++i)
    {
        int index =
            static_cast<int>(i * step);

        result.append(history.at(index));
    }

    return result;
}

void DeviceWidget::setupHistoryCharts()
{

    // 历史温度图


    m_historyTemperatureSeries =
        new QLineSeries();

    QChart *temperatureChart =
        new QChart();

    temperatureChart->addSeries(
        m_historyTemperatureSeries
        );

    temperatureChart->setTitle(
        "温度历史趋势"
        );

    temperatureChart->legend()->hide();

    temperatureChart->setMargins(
        QMargins(10, 10, 10, 10)
        );

    m_historyTemperatureAxisX =
        new QDateTimeAxis();

    m_historyTemperatureAxisX->setFormat(
        "HH:mm:ss"
        );

    m_historyTemperatureAxisX->setTitleText(
        "时间"
        );

    m_historyTemperatureAxisX->setTickCount(6);

    m_historyTemperatureAxisY =
        new QValueAxis();

    m_historyTemperatureAxisY->setTitleText(
        "温度 (°C)"
        );

    m_historyTemperatureAxisY->setLabelFormat(
        "%.1f"
        );

    m_historyTemperatureAxisY->setTickCount(5);

    m_historyTemperatureAxisY->setRange(
        15,
        65
        );

    temperatureChart->addAxis(
        m_historyTemperatureAxisX,
        Qt::AlignBottom
        );

    temperatureChart->addAxis(
        m_historyTemperatureAxisY,
        Qt::AlignLeft
        );

    m_historyTemperatureSeries->attachAxis(
        m_historyTemperatureAxisX
        );

    m_historyTemperatureSeries->attachAxis(
        m_historyTemperatureAxisY
        );

    m_historyTemperatureChartView =
        new QChartView(
            temperatureChart
            );

    m_historyTemperatureChartView->setRenderHint(
        QPainter::Antialiasing
        );

    // 历史电压图

    m_historyVoltageSeries =
        new QLineSeries();

    QChart *voltageChart =
        new QChart();

    voltageChart->addSeries(
        m_historyVoltageSeries
        );

    voltageChart->setTitle(
        "电压历史趋势"
        );

    voltageChart->legend()->hide();

    voltageChart->setMargins(
        QMargins(10, 10, 10, 10)
        );

    m_historyVoltageAxisX =
        new QDateTimeAxis();

    m_historyVoltageAxisX->setFormat(
        "HH:mm:ss"
        );

    m_historyVoltageAxisX->setTitleText(
        "时间"
        );

    m_historyVoltageAxisX->setTickCount(6);

    m_historyVoltageAxisY =
        new QValueAxis();

    m_historyVoltageAxisY->setTitleText(
        "电压 (V)"
        );

    m_historyVoltageAxisY->setLabelFormat(
        "%.1f"
        );

    m_historyVoltageAxisY->setTickCount(5);

    m_historyVoltageAxisY->setRange(
        210,
        230
        );

    voltageChart->addAxis(
        m_historyVoltageAxisX,
        Qt::AlignBottom
        );

    voltageChart->addAxis(
        m_historyVoltageAxisY,
        Qt::AlignLeft
        );

    m_historyVoltageSeries->attachAxis(
        m_historyVoltageAxisX
        );

    m_historyVoltageSeries->attachAxis(
        m_historyVoltageAxisY
        );

    m_historyVoltageChartView =
        new QChartView(
            voltageChart
            );

    m_historyVoltageChartView->setRenderHint(
        QPainter::Antialiasing
        );

    // 放入布局
    // 把温度图放进 UI
    QLayout *temperatureLayout =
        ui->historyTemperatureChartWidget->layout();

    temperatureLayout->setContentsMargins(0, 0, 0, 0);
    temperatureLayout->addWidget(m_historyTemperatureChartView);

    // 把电压图放进 UI
    QLayout *voltageLayout =
        ui->historyVoltageChartWidget->layout();

    voltageLayout->setContentsMargins(0, 0, 0, 0);
    voltageLayout->addWidget(m_historyVoltageChartView);

    qDebug() << "温度Widget:"
             << ui->historyTemperatureChartWidget->size();

    qDebug() << "电压Widget:"
             << ui->historyVoltageChartWidget->size();

    qDebug() << "温度Chart:"
             << m_historyTemperatureChartView->size();

    qDebug() << "电压Chart:"
             << m_historyVoltageChartView->size();
}

void DeviceWidget::updateHistoryCharts(
    const QList<DeviceHistory> &history)
{
    m_historyTemperatureSeries->clear();
    m_historyVoltageSeries->clear();

    if (history.isEmpty())
    {
        qDebug() << "没有查询到历史数据";
        return;
    }

    double minTemperature =
        history.first().data.temperature;

    double maxTemperature =
        history.first().data.temperature;

    double minVoltage =
        history.first().data.voltage;

    double maxVoltage =
        history.first().data.voltage;

    for (const DeviceHistory &item : history)
    {
        qint64 timestamp =
            item.timestamp.toMSecsSinceEpoch();

        double temperature =
            item.data.temperature;

        double voltage =
            item.data.voltage;

        m_historyTemperatureSeries->append(
            timestamp,
            temperature
            );

        m_historyVoltageSeries->append(
            timestamp,
            voltage
            );

        minTemperature =
            qMin(minTemperature, temperature);

        maxTemperature =
            qMax(maxTemperature, temperature);

        minVoltage =
            qMin(minVoltage, voltage);

        maxVoltage =
            qMax(maxVoltage, voltage);
    }

    // X轴
    QDateTime startTime =
        history.first().timestamp;

    QDateTime endTime =
        history.last().timestamp;

    if (startTime == endTime)
        endTime = endTime.addSecs(1);

    m_historyTemperatureAxisX->setRange(
        startTime,
        endTime
        );

    m_historyVoltageAxisX->setRange(
        startTime,
        endTime
        );

    // Y轴
    double temperatureMargin =
        qMax(
            1.0,
            (maxTemperature - minTemperature) * 0.1
            );

    double voltageMargin =
        qMax(
            0.5,
            (maxVoltage - minVoltage) * 0.1
            );

    m_historyTemperatureAxisY->setRange(
        minTemperature - temperatureMargin,
        maxTemperature + temperatureMargin
        );

    m_historyVoltageAxisY->setRange(
        minVoltage - voltageMargin,
        maxVoltage + voltageMargin
        );
}
