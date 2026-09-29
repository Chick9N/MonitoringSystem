#include "modbusrtu.h"

ModbusRTU::ModbusRTU() {}

// 计算 Modbus RTU CRC-16
quint16 ModbusRTU::calculateCRC(const QByteArray &data)
{
    quint16 crc = 0xFFFF;

    for (unsigned char byte : data)
    {
        crc ^= byte;

        for (int i = 0; i < 8; ++i)
        {
            if (crc & 0x0001)
            {
                crc >>= 1;
                crc ^= 0xA001;
            }
            else
            {
                crc >>= 1;
            }
        }
    }

    return crc;
}

QByteArray ModbusRTU::buildReadHoldingRegistersRequest(
    quint8 slaveAddress,
    quint16 startAddress,
    quint16 quantity)
{
    // 检查读取数量是否合法
    if (quantity < 1 || quantity > 125)
    {
        return QByteArray();
    }

    QByteArray request;

    // 从站地址
    request.append(static_cast<char>(slaveAddress));

    // 功能码：读取保持寄存器
    request.append(static_cast<char>(0x03));

    // 起始寄存器地址（高字节在前）
    request.append(static_cast<char>((startAddress >> 8) & 0xFF));
    request.append(static_cast<char>(startAddress & 0xFF));

    // 读取寄存器数量（高字节在前）
    request.append(static_cast<char>((quantity >> 8) & 0xFF));
    request.append(static_cast<char>(quantity & 0xFF));

    // 计算 CRC
    quint16 crc = calculateCRC(request);

    // CRC 低字节在前
    request.append(static_cast<char>(crc & 0xFF));
    request.append(static_cast<char>((crc >> 8) & 0xFF));

    return request;
}

bool ModbusRTU::parseReadHoldingRegistersResponse(
    const QByteArray &response,
    quint8 expectedSlave,
    quint16 expectedQuantity,
    QVector<quint16> &registers,
    QString &error)
{
    registers.clear();
    error.clear();

    // 检查预期寄存器数量
    if (expectedQuantity < 1 || expectedQuantity > 125)
    {
        error = "Invalid expected quantity";
        return false;
    }

    // 最小响应长度：地址 + 功能码 + 字节数 + CRC
    if (response.size() < 5)
    {
        error = "Response too short";
        return false;
    }

    quint8 slave = static_cast<quint8>(
        static_cast<unsigned char>(response[0]));

    quint8 function = static_cast<quint8>(
        static_cast<unsigned char>(response[1]));

    // 检查从站地址
    if (slave != expectedSlave)
    {
        error = "Slave address mismatch";
        return false;
    }

    // 检查异常响应
    if (function == 0x83)
    {
        if (response.size() != 5)
        {
            error = "Invalid exception response length";
            return false;
        }

        quint16 receivedCRC =
            static_cast<quint8>(
                static_cast<unsigned char>(response[3])) |
            (static_cast<quint16>(
                 static_cast<quint8>(
                     static_cast<unsigned char>(response[4]))) << 8);

        quint16 calculatedCRC =
            calculateCRC(response.left(response.size() - 2));

        if (receivedCRC != calculatedCRC)
        {
            error = "CRC mismatch";
            return false;
        }

        quint8 exceptionCode = static_cast<quint8>(
            static_cast<unsigned char>(response[2]));

        error = QString("Modbus exception: 0x%1")
                    .arg(exceptionCode, 2, 16, QChar('0'))
                    .toUpper();

        return false;
    }

    // 检查功能码
    if (function != 0x03)
    {
        error = "Function code mismatch";
        return false;
    }

    // 检查数据字节数
    quint8 byteCount = static_cast<quint8>(
        static_cast<unsigned char>(response[2]));

    if (byteCount != expectedQuantity * 2)
    {
        error = "Register byte count mismatch";
        return false;
    }

    // 检查响应帧长度
    if (response.size() != 3 + byteCount + 2)
    {
        error = "Response length mismatch";
        return false;
    }

    // 提取接收到的 CRC（低字节在前）
    int crcIndex = response.size() - 2;

    quint16 receivedCRC =
        static_cast<quint8>(
            static_cast<unsigned char>(response[crcIndex])) |
        (static_cast<quint16>(
             static_cast<quint8>(
                 static_cast<unsigned char>(response[crcIndex + 1]))) << 8);

    // 重新计算 CRC
    quint16 calculatedCRC =
        calculateCRC(response.left(response.size() - 2));

    if (receivedCRC != calculatedCRC)
    {
        error = "CRC mismatch";
        return false;
    }

    // 解析寄存器数据（高字节在前）
    for (int i = 0; i < byteCount; i += 2)
    {
        quint16 value =
            (static_cast<quint8>(
                 static_cast<unsigned char>(response[3 + i])) << 8) |
            static_cast<quint8>(
                static_cast<unsigned char>(response[4 + i]));

        registers.append(value);
    }

    return true;
}

bool ModbusRTU::tryExtractResponseFrame(
    QByteArray &buffer,
    quint8 expectedSlave,
    quint16 expectedQuantity,
    QByteArray &frame)
{
    frame.clear();

    if (expectedQuantity < 1 || expectedQuantity > 125)
        return false;

    while (true)
    {
        // 至少需要从站地址和功能码
        if (buffer.size() < 2)
            return false;

        // 查找当前请求对应的从站地址
        int slaveIndex = buffer.indexOf(
            static_cast<char>(expectedSlave)
            );

        // 缓冲区中没有对应地址，清除无效数据
        if (slaveIndex == -1)
        {
            buffer.clear();
            return false;
        }

        // 丢弃从站地址之前的无效数据
        if (slaveIndex > 0)
            buffer.remove(0, slaveIndex);

        // 地址后至少需要一个功能码
        if (buffer.size() < 2)
            return false;

        quint8 function = static_cast<quint8>(
            static_cast<unsigned char>(buffer[1])
            );

        int frameLength = 0;

        // 异常响应：地址 + 异常功能码 + 异常码 + CRC
        if (function == 0x83)
        {
            frameLength = 5;
        }
        // 正常读取保持寄存器响应
        else if (function == 0x03)
        {
            // 等待字节数
            if (buffer.size() < 3)
                return false;

            quint8 byteCount = static_cast<quint8>(
                static_cast<unsigned char>(buffer[2])
                );

            // 字节数与当前请求不符，丢弃当前候选地址
            if (byteCount != expectedQuantity * 2)
            {
                buffer.remove(0, 1);
                continue;
            }

            frameLength = 3 + byteCount + 2;
        }
        else
        {
            // 不支持的功能码，跳过当前候选地址
            buffer.remove(0, 1);
            continue;
        }

        // 等待完整帧到达
        if (buffer.size() < frameLength)
            return false;

        // 提取候选完整帧
        QByteArray candidate = buffer.left(frameLength);

        // 检查 CRC，避免将噪声误识别为完整响应
        quint16 receivedCRC =
            static_cast<quint8>(
                static_cast<unsigned char>(
                    candidate[frameLength - 2]
                    )
                ) |
            (static_cast<quint16>(
                 static_cast<quint8>(
                     static_cast<unsigned char>(
                         candidate[frameLength - 1]
                         )
                     )
                 ) << 8);

        quint16 calculatedCRC =
            calculateCRC(candidate.left(frameLength - 2));

        if (receivedCRC != calculatedCRC)
        {
            // CRC 错误，移动一个字节重新同步
            buffer.remove(0, 1);
            continue;
        }

        // CRC 正确，提取完整帧
        frame = candidate;
        buffer.remove(0, frameLength);

        return true;
    }
}
