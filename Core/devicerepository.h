#ifndef DEVICEREPOSITORY_H
#define DEVICEREPOSITORY_H

class DeviceRepository
{
public:
    virtual ~DeviceRepository() = default;
    virtual bool deleteDevice(int deviceId) = 0;
};

#endif // DEVICEREPOSITORY_H
