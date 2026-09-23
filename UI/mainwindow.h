#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include "../Core/devicemanager.h"
#include "../Data/databasemanager.h"
#include "devicewidget.h"

#include <QTimer>
#include <QMap>
#include <QList>
#include <QMainWindow>


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    int findCurrentAlarm(
        int deviceId,
        AlarmType type) const;
    void loadAlarmHistory();
    void loadCurrentAlarms();
    void updateAlarmStatistics();
private:
    Ui::MainWindow *ui;
    DeviceManager *deviceManager;
    DatabaseManager *databaseManager;
    QTimer *timer;
    QMap<int, DeviceWidget*> m_deviceWidgets; // 判断某个设备有没有已经打开的详情窗口
    QMap<int, QList<double>> m_temperatureHistory;
    QMap<int, QList<double>> m_voltageHistory;
    static constexpr int MaxHistoryPoints = 30;

private slots:
    void updateDeviceUI(int deviceId,const DeviceData &data);
    void on_startAllBtn_clicked();
    void on_stopAllBtn_clicked();
    void on_deviceTable_cellDoubleClicked(int row, int column);

    void handleAlarm(const AlarmInfo &alarm);
};

#endif // MAINWINDOW_H
