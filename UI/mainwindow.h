#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "../Core/devicemanager.h"
#include "devicewidget.h"
#include <QTimer>
#include <QMap>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    DeviceManager *deviceManager;
    QTimer *timer;
    QMap<int, DeviceWidget*> m_deviceWidgets; // 判断某个设备有没有已经打开的详情窗口
private slots:
    void updateDeviceUI(int deviceId,const DeviceData &data);
    void on_startAllBtn_clicked();
    void on_stopAllBtn_clicked();
    void on_deviceTable_cellDoubleClicked(int row, int column);
};

#endif // MAINWINDOW_H
