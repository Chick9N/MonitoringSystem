#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include "../Core/devicemanager.h"
#include "../Data/databasemanager.h"
#include "../Data/databasewritequeue.h"
#include "../Communication/tcpserverservice.h"
#include "devicewidget.h"


#include <QTimer>
#include <QMap>
#include <QList>
#include <QMainWindow>
#include <QFutureWatcher>

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
    void updateAlarmStatistics();
private:
    Ui::MainWindow *ui;
    DeviceManager *deviceManager;
    DatabaseManager *databaseManager;
    DatabaseWriteQueue *databaseWriteQueue;
    QTimer *timer;
    QMap<int,int> m_deviceRowMap; // 表格映射 设备ID, 表格行
    QMap<int, DeviceWidget*> m_deviceWidgets; // 判断某个设备有没有已经打开的详情窗口
    QMap<int, QList<double>> m_temperatureHistory;
    QMap<int, QList<double>> m_voltageHistory;
    static constexpr int MaxHistoryPoints = 30;
    TcpServerService *m_tcpServerService;
    QFutureWatcher<DatabaseSnapshot> *m_startupWatcher = nullptr;
    void applyStartupData(const DatabaseSnapshot &data);

private slots:
    void removeDeviceRow(int deviceId);
    void updateDeviceUI(int deviceId,const DeviceData &data);
    void on_startAllBtn_clicked();
    void on_stopAllBtn_clicked();
    void on_deviceTable_cellDoubleClicked(int row, int column);

    void handleAlarm(const AlarmInfo &alarm);
    void on_mockSerialConfigBtn_clicked();

    void addDeviceRow(int deviceId);
    void on_testBtn_clicked();
    void on_addDeviceBtn_clicked();
    void on_tcpServerBtn_clicked();
};

#endif // MAINWINDOW_H
