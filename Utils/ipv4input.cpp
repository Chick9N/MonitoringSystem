#include "ipv4input.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QIntValidator>
#include <QKeyEvent>
#include <QClipboard>
#include <QApplication>
#include <QHostAddress>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QEvent>

IPv4Input::IPv4Input(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(0);

    for (int i = 0; i < 4; ++i) {
        m_octets[i] = new QLineEdit(this);
        m_octets[i]->setAlignment(Qt::AlignCenter);
        m_octets[i]->setMaxLength(3);
        m_octets[i]->setFixedSize(50, 30);
        m_octets[i]->setValidator(
            new QIntValidator(0, 255, m_octets[i])
            );

        m_octets[i]->installEventFilter(this);
        layout->addWidget(m_octets[i]);

        if (i < 3) {
            auto *dot = new QLabel(".", this);
            dot->setFixedSize(14, 30);
            dot->setAlignment(Qt::AlignCenter);
            layout->addWidget(dot);
        }
    }

    setMinimumSize(256, 34);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setAddress("127.0.0.1");
}

QString IPv4Input::address() const
{
    return QString("%1.%2.%3.%4")
    .arg(m_octets[0]->text())
        .arg(m_octets[1]->text())
        .arg(m_octets[2]->text())
        .arg(m_octets[3]->text());
}

bool IPv4Input::isValid() const
{
    for (int i = 0; i < 4; ++i) {
        const QString text = m_octets[i]->text();

        if (text.isEmpty())
            return false;

        bool ok = false;
        int value = text.toInt(&ok);

        if (!ok || value < 0 || value > 255)
            return false;
    }

    return true;
}

void IPv4Input::setAddress(const QString &ip)
{
    QHostAddress addr;

    if (!addr.setAddress(ip) ||
        addr.protocol() != QAbstractSocket::IPv4Protocol) {
        return;
    }

    setAddressParts(ip.split('.'));
}

bool IPv4Input::setAddressParts(const QStringList &parts)
{
    if (parts.size() != 4)
        return false;

    for (const QString &part : parts) {
        bool ok = false;
        int value = part.toInt(&ok);

        if (!ok || part.isEmpty() || value < 0 || value > 255)
            return false;
    }

    for (int i = 0; i < 4; ++i) {
        m_octets[i]->setText(QString::number(parts[i].toInt()));
    }

    updateAddress();
    return true;
}

void IPv4Input::clear()
{
    for (auto *octet : m_octets)
        octet->clear();

    focusOctet(0);
    updateAddress();
}

void IPv4Input::focusOctet(int index)
{
    if (index < 0 || index > 3)
        return;

    m_octets[index]->setFocus();
    m_octets[index]->selectAll();
}

void IPv4Input::updateAddress()
{
    emit addressChanged(address());
}

bool IPv4Input::eventFilter(QObject *obj, QEvent *event)
{
    int index = -1;

    for (int i = 0; i < 4; ++i) {
        if (obj == m_octets[i]) {
            index = i;
            break;
        }
    }

    if (index < 0 || event->type() != QEvent::KeyPress)
        return QWidget::eventFilter(obj, event);

    auto *keyEvent = static_cast<QKeyEvent *>(event);

    // 输入点号，跳转到下一段
    if (keyEvent->key() == Qt::Key_Period ||
        keyEvent->key() == Qt::Key_Comma) {
        if (index < 3)
            focusOctet(index + 1);
        return true;
    }

    // 当前段为空时，退格返回上一段
    if (keyEvent->key() == Qt::Key_Backspace &&
        m_octets[index]->text().isEmpty() && index > 0) {
        focusOctet(index - 1);
        return true;
    }

    // 粘贴完整 IPv4 地址
    if ((keyEvent->modifiers() & Qt::ControlModifier) &&
        keyEvent->key() == Qt::Key_V) {
        const QString pasted = QApplication::clipboard()->text().trimmed();

        static const QRegularExpression ipv4Regex(
            R"(^\d{1,3}(\.\d{1,3}){3}$)"
            );

        if (ipv4Regex.match(pasted).hasMatch()) {
            if (setAddressParts(pasted.split('.'))) {
                focusOctet(3);
                return true;
            }
        }
    }

    return QWidget::eventFilter(obj, event);
}

QSize IPv4Input::sizeHint() const
{
    return QSize(250, 26);
}
