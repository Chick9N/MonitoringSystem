#ifndef SIMULATEDATA_H
#define SIMULATEDATA_H


#include <QtGlobal>


class SimulationData
{

public:

    // 温度模拟
    // 单位: 0.1℃
    static quint16 temperature(
        quint16 current
        );


    // 电压模拟
    // 单位: V
    static quint16 voltage(
        quint16 current
        );


    // 在线状态模拟
    static quint16 online();


private:

    static int randomRange(
        int min,
        int max
        );

};


#endif // SIMULATEDATA_H
