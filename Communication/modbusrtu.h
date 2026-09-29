#ifndef MODBUSRTU_H
#define MODBUSRTU_H

#include <QByteArray>
#include <QtGlobal>
#include <QVector>
#include <QString>

class ModbusRTU
{
public:
    ModbusRTU();

    // 计算 Modbus RTU CRC-16
    static quint16 calculateCRC(const QByteArray &data);

    // 构造读取保持寄存器请求帧（功能码 0x03）
    static QByteArray buildReadHoldingRegistersRequest(
        quint8 slaveAddress,
        quint16 startAddress,
        quint16 quantity
        );

    // 解析功能码 0x03 的正常响应
    static bool parseReadHoldingRegistersResponse(
        const QByteArray &response,
        quint8 expectedSlave,
        quint16 expectedQuantity,
        QVector<quint16> &registers,
        QString &error
        );

    // 从接收缓冲区提取一帧 0x03 响应
    // 返回 true 表示成功提取一帧
    // 返回 false 表示当前数据不足以提取完整帧
    static bool tryExtractResponseFrame(
        QByteArray &buffer,
        quint8 expectedSlave,
        quint16 expectedQuantity,
        QByteArray &frame
        );
};

#endif // MODBUSRTU_H
