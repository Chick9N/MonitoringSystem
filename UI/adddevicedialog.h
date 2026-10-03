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
private slots:
    void on_protocolComboBox_currentIndexChanged(int index);

    void on_buttonBox_accepted();

    void on_buttonBox_rejected();

protected:
    void accept() override;
private:
    Ui::AddDeviceDialog *ui;
};

#endif // ADDDEVICEDIALOG_H
