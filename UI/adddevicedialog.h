#ifndef ADDDEVICEDIALOG_H
#define ADDDEVICEDIALOG_H

#include <QDialog>
#include <QString>
#include "../Core/deviceconfig.h"

namespace Ui {
class AddDeviceDialog;
}

class AddDeviceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddDeviceDialog(QWidget *parent = nullptr);
    ~AddDeviceDialog();
    DeviceConfig getDeviceConfig() const;
public slots:
    void onTCPTestResult(
        bool success,
        const QString &message);

private slots:
    void on_protocolComboBox_currentIndexChanged(int index);

    void on_buttonBox_accepted();

    void on_buttonBox_rejected();

    void on_testConnectionBtn_clicked();

signals:
    // 测试通信用信号
    void testTCPConnection(
        const QString &ip,
        quint16 port
        );

protected:
    void accept() override;
private:
    Ui::AddDeviceDialog *ui;
};

#endif // ADDDEVICEDIALOG_H
