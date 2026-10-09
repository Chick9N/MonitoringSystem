#ifndef MOCKSERIALCONFIGWINDOW_H
#define MOCKSERIALCONFIGWINDOW_H

#include <QWidget>

class QShowEvent;

class MockSerialDevice;
class MockModbusRTUDevice;

namespace Ui {
class MockSerialConfigWindow;
}

class MockSerialConfigWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MockSerialConfigWindow(QWidget *parent = nullptr);
    ~MockSerialConfigWindow();

private slots:
    void on_refreshSerialButton_clicked();
    void on_openSerialButton_clicked();
    void on_closeSerialButton_clicked();

    void on_modbusModeCheckBox_stateChanged(int arg1);

private:
    void refreshSerialPorts();
    void updateSerialStatus();

private:
    Ui::MockSerialConfigWindow *ui;
    MockSerialDevice *m_mockDevice;
    MockModbusRTUDevice *m_modbusDevice;
    bool m_modbusMode = false;

protected:
    void showEvent(QShowEvent *event) override;
};

#endif // MOCKSERIALCONFIGWINDOW_H
