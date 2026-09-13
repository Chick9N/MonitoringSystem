#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "../Core/devicemanager.h"
#include <QTimer>
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

private slots:
    void updateDeviceUI(int deviceId,const DeviceData &data);
    void on_startAllBtn_clicked();
    void on_stopAllBtn_clicked();
};

#endif // MAINWINDOW_H
