#ifndef SERIALCONFIGWINDOW_H
#define SERIALCONFIGWINDOW_H

#include <QWidget>
#include "../Core/devicemanager.h"

namespace Ui {
class SerialConfigWindow;
}

class SerialConfigWindow : public QWidget
{
    Q_OBJECT

public:
    explicit SerialConfigWindow(DeviceManager *deviceManager,QWidget *parent = nullptr);
    ~SerialConfigWindow();

private slots:
    void on_refreshSerialButton_clicked();

    void on_openSerialButton_clicked();

    void on_closeSerialButton_clicked();

private:
    Ui::SerialConfigWindow *ui;
    DeviceManager *m_deviceManager;

    void refreshSerialPorts();
    void updateSerialStatus();
};

#endif // SERIALCONFIGWINDOW_H
