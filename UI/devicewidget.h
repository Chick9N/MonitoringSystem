#ifndef DEVICEWIDGET_H
#define DEVICEWIDGET_H

#include <QWidget>
#include "../Data/databasemanager.h"
#include "../Core/device.h"
namespace Ui {
class DeviceWidget;
}

class QChartView;
class QLineSeries;
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

private:
    void setupCharts();
    void loadHistory();
    void updateCharts(const DeviceData &data);
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
};

#endif // DEVICEWIDGET_H
