#ifndef MOCKSERIALCONFIGWINDOW_H
#define MOCKSERIALCONFIGWINDOW_H

#include <QWidget>

class MockSerialDevice;

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

private:
    void refreshSerialPorts();
    void updateSerialStatus();

private:
    Ui::MockSerialConfigWindow *ui;
    MockSerialDevice *m_mockDevice;
};

#endif // MOCKSERIALCONFIGWINDOW_H
