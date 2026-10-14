#ifndef DEVICEWIDGET_H
#define DEVICEWIDGET_H

#include <QWidget>
#include "../Data/databasemanager.h"
#include "../Core/device.h"
#include <QFutureWatcher>
namespace Ui {
class DeviceWidget;
}

class QLineSeries;
class QChartView;
class QValueAxis;
class QDateTimeAxis;

class DeviceWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DeviceWidget(
        Device *device,
        DatabaseManager *databaseManager,
        QWidget *parent = nullptr);
    ~DeviceWidget();

private slots:
    void updateWidget(const DeviceData &data);
    void on_queryHistoryButton_clicked();

private:
    // 设备管理器和设备的生命周期长于详情窗口；图表对象由窗口持有。
    Ui::DeviceWidget *m_ui = nullptr;
    Device *m_device = nullptr;
    DatabaseManager *m_databaseManager = nullptr;
    QFutureWatcher<QList<DeviceHistory>> *m_historyWatcher = nullptr;

    // 实时图表控件、坐标轴和数据序列。
    QChartView *m_temperatureChartView = nullptr;
    QChartView *m_voltageChartView = nullptr;
    QDateTimeAxis *m_temperatureAxisX = nullptr;
    QValueAxis *m_temperatureAxisY = nullptr;
    QDateTimeAxis *m_voltageAxisX = nullptr;
    QValueAxis *m_voltageAxisY = nullptr;
    QLineSeries *m_temperatureSeries = nullptr;
    QLineSeries *m_voltageSeries = nullptr;

    // 历史图表使用独立数据序列，查询历史区间时不会覆盖实时曲线。
    QChartView *m_historyTemperatureChartView = nullptr;
    QChartView *m_historyVoltageChartView = nullptr;
    QDateTimeAxis *m_historyTemperatureAxisX = nullptr;
    QDateTimeAxis *m_historyVoltageAxisX = nullptr;
    QValueAxis *m_historyTemperatureAxisY = nullptr;
    QValueAxis *m_historyVoltageAxisY = nullptr;
    QLineSeries *m_historyTemperatureSeries = nullptr;
    QLineSeries *m_historyVoltageSeries = nullptr;

    void setupCharts();
    void updateCharts(const DeviceData &data);
    void setupHistoryCharts();
    void updateHistoryCharts(const QList<DeviceHistory> &history);
    QList<DeviceHistory> sampleHistory(const QList<DeviceHistory> &history, int maxPoints);
};

#endif // DEVICEWIDGET_H
