#ifndef DEVICEWIDGET_H
#define DEVICEWIDGET_H

#include <QWidget>
#include "../Data/databasemanager.h"
#include "../Core/device.h"
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
    explicit DeviceWidget(Device *device,DatabaseManager *databaseManager,QWidget *parent = nullptr);
    ~DeviceWidget();

private slots:
    void updateWidget(const DeviceData &data);

    void on_startBtn_clicked();
    void on_stopBtn_clicked();


    void on_queryHistoryButton_clicked();

private:
    void setupCharts();
    void loadHistory();
    void updateCharts(const DeviceData &data);
    void setupHistoryCharts();
    void updateHistoryCharts(const QList<DeviceHistory> &history);
    QList<DeviceHistory> sampleHistory(
        const QList<DeviceHistory> &history,
        int maxPoints);
    Ui::DeviceWidget *ui;
    Device *m_device;
    DatabaseManager *m_databaseManager;

    QChartView *m_temperatureChartView;
    QChartView *m_voltageChartView;

    QDateTimeAxis *m_temperatureAxisX;
    QValueAxis *m_temperatureAxisY;
    QDateTimeAxis *m_voltageAxisX;
    QValueAxis *m_voltageAxisY;

    QLineSeries *m_temperatureSeries;
    QLineSeries *m_voltageSeries;

    QLineSeries *m_historyTemperatureSeries;
    QLineSeries *m_historyVoltageSeries;

    QChartView *m_historyTemperatureChartView;
    QChartView *m_historyVoltageChartView;

    QDateTimeAxis *m_historyTemperatureAxisX;
    QDateTimeAxis *m_historyVoltageAxisX;

    QValueAxis *m_historyTemperatureAxisY;
    QValueAxis *m_historyVoltageAxisY;
};

#endif // DEVICEWIDGET_H
