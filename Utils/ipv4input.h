#ifndef IPV4INPUT_H
#define IPV4INPUT_H

#include <QWidget>
#include <QLineEdit>
#include <QSize>

class IPv4Input : public QWidget
{
    Q_OBJECT

public:
    explicit IPv4Input(QWidget *parent = nullptr);

    QString address() const;
    bool isValid() const;
    void setAddress(const QString &ip);
    void clear();

    QSize sizeHint() const override;

signals:
    void addressChanged(const QString &ip);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    QLineEdit *m_octets[4];

    void focusOctet(int index);
    void updateAddress();
    bool setAddressParts(const QStringList &parts);
};

#endif
