#ifndef DEVICEWIDGET_H
#define DEVICEWIDGET_H

#include <QWidget>
#include "../Core/device.h"
namespace Ui {
class DeviceWidget;
}

class DeviceWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DeviceWidget(Device *device,QWidget *parent = nullptr);
    ~DeviceWidget();

private slots:
    void updateWidget(const DeviceData &data);

    void on_startBtn_clicked();
    void on_stopBtn_clicked();

private:
    Ui::DeviceWidget *ui;
    Device *m_device;
};

#endif // DEVICEWIDGET_H
