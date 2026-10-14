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

protected:
    void showEvent(QShowEvent *event) override;

private:
    void refreshSerialPorts();
    void updateSerialStatus();

    // Keep both simulators alive while the window is hidden.
    Ui::MockSerialConfigWindow *m_ui = nullptr;
    MockSerialDevice *m_mockDevice = nullptr;
    MockModbusRTUDevice *m_modbusDevice = nullptr;
    bool m_modbusMode = false;
};

#endif // MOCKSERIALCONFIGWINDOW_H
