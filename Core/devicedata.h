#ifndef DEVICEDATA_H
#define DEVICEDATA_H

struct DeviceData
{
    DeviceData() {};
    double temperature = 0.0;
    double voltage = 0.0;
    bool isOnline = false;
};

#endif // DEVICEDATA_H
