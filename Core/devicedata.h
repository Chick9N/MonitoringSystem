#ifndef DEVICEDATA_H
#define DEVICEDATA_H
#include <QString>

struct DeviceData
{
    DeviceData() {};
    bool isOnline = false;
    double temperature = 0.0;
    double voltage = 0.0;
};

#endif // DEVICEDATA_H
