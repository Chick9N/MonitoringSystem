#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include "../Core/devicemanager.h"
#include "../Data/databasemanager.h"
#include "../Data/databasewritequeue.h"
#include "../Communication/tcpserverservice.h"
#include "devicewidget.h"


#include <QFutureWatcher>
#include <QList>
#include <QMap>
#include <QMainWindow>
#include <QPointer>
#include <QTimer>

namespace Ui {
class MainWindow;
}

class SystemTestDialog;
class MockSerialConfigWindow;
class QLabel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // 设备生命周期、表格行和详情窗口。
    // 这些槽函数需同步维护行号映射和缓存的历史数据。
    void addDeviceRow(int deviceId);
    void removeDeviceRow(int deviceId);
    void updateDeviceUI(int deviceId, const DeviceData &data);
    void on_addDeviceBtn_clicked();
    void on_deviceTable_cellDoubleClicked(int row, int column);
    void on_startAllBtn_clicked();
    void on_stopAllBtn_clicked();

    // 报警状态与确认操作；当前报警和历史记录的生命周期不同。
    void handleAlarm(const AlarmInfo &alarm);
    void on_acknowledgeAlarmBtn_clicked();

    // 通信工具和诊断功能。
    void on_mockSerialConfigBtn_clicked();
    void on_tcpServerBtn_clicked();
    void on_runSelfTestBtn_clicked();
    void on_openLogsBtn_clicked();

private:
    // Qt Designer 控件及主窗口使用的服务对象。
    Ui::MainWindow *m_ui = nullptr;
    DatabaseManager *m_databaseManager = nullptr;
    DeviceManager *m_deviceManager = nullptr;
    DatabaseWriteQueue *m_databaseWriteQueue = nullptr;
    TcpServerService *m_tcpServerService = nullptr;

    // 定时更新与启动数据加载。
    QTimer *m_updateTimer = nullptr;
    QTimer *m_alarmBlinkTimer = nullptr;
    QFutureWatcher<DatabaseSnapshot> *m_startupWatcher = nullptr;

    // 后台读取完成后，在界面线程中应用启动数据快照。
    void applyStartupData(const DatabaseSnapshot &data);

    // 设备界面状态和有长度上限的曲线历史数据。
    void filterDevices(const QString &query);
    QMap<int, int> m_deviceRowMap; // 设备 ID 到当前表格行号的映射。
    QMap<int, DeviceWidget *> m_deviceWidgets; // 按设备 ID 管理已打开的详情窗口。
    QMap<int, QList<double>> m_temperatureHistory;
    QMap<int, QList<double>> m_voltageHistory;
    static constexpr int m_maxHistoryPoints = 30;

    // 报警显示辅助函数。
    int findCurrentAlarm(int deviceId, AlarmType type) const;
    void updateAlarmStatistics();
    void updateAlarmBlinking();
    bool m_alarmBlinkPhase = false;

    // 连接指示灯表示通信通道状态，设备行状态表示数据是否仍然有效。
    void setConnectionIndicator(QLabel *indicator, bool active);

    // 辅助窗口打开期间保留状态；窗口销毁时 QPointer 会自动清空。
    MockSerialConfigWindow *m_mockSerialConfigWindow = nullptr;
    QPointer<SystemTestDialog> m_systemTestDialog;
    bool m_simulationRunning = false;
};

#endif // MAINWINDOW_H
