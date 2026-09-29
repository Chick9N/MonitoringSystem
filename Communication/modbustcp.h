#ifndef MODBUSTCP_H
#define MODBUSTCP_H

#include <QTcpSocket>
#include <QByteArray>
#include <QObject>
#include <QVector>
#include <QString>

class ModbusTCP : public QObject
{
    Q_OBJECT
public:
    ModbusTCP(QObject *parent = nullptr);
    static QByteArray buildReadHoldingRegistersRequest(
        quint16 transactionId,
        quint8 unitId,
        quint16 startAddress,
        quint16 quantity
        );

    static bool parseReadHoldingRegistersResponse(
        const QByteArray &response,
        quint8 expectedUnit,
        quint16 expectedTransaction,
        quint16 expectedQuantity,
        QVector<quint16> &registers,
        QString &error
        );

    bool sendReadHoldingRegistersRequest(
        quint16 transactionId,
        quint8 unitId,
        quint16 startAddress,
        quint16 quantity
        );

    void connectToDevice(const QString &ip, quint16 port);
    void disconnectFromDevice();
    bool sendData(const QByteArray &data);

signals:
    void dataReceived(const QByteArray &data);

    // TCP接收到的原始数据
    void rawDataReceived(const QByteArray &data);

    void connected();

    void disconnected();

    void errorOccurred(const QString &message);
private:
    QTcpSocket *m_socket;
    QByteArray m_buffer;
    // 从 TCP 缓冲区提取完整 Modbus TCP 帧
    bool tryExtractFrame(QByteArray &frame);


private slots:

    void readData();
};

#endif // MODBUSTCP_H
