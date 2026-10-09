#include "simulatedata.h"

#include <QRandomGenerator>



quint16 SimulationData::temperature(
    quint16 current)
{
    // 每隔若干采样点制造短暂越界，便于验证高低温报警与恢复流程。
    const int excursion = randomRange(0, 23);
    if (excursion == 0)
        return static_cast<quint16>(randomRange(520, 580));
    if (excursion == 1)
        return static_cast<quint16>(randomRange(90, 140));

    int delta =
        randomRange(-5,5);


    int value =
        current + delta;


    if(value < 150)
        value = 150;


    if(value > 500)
        value = 500;


    return value;
}




quint16 SimulationData::voltage(
    quint16 current)
{
    const int excursion = randomRange(0, 19);
    if (excursion == 0)
        return static_cast<quint16>(randomRange(235, 245));
    if (excursion == 1)
        return static_cast<quint16>(randomRange(195, 205));

    int delta =
        randomRange(-1,1);



    int value =
        static_cast<int>(current)
        + delta;



    if(value < 210)
        value = 210;


    if(value > 230)
        value = 230;


    return static_cast<quint16>(value);

}




quint16 SimulationData::online()
{

    /*
        暂时固定在线

        后续可以扩展:
        随机掉线
        心跳超时

    */


    return 1;

}




int SimulationData::randomRange(
    int min,
    int max)
{

    return
        QRandomGenerator::global()
            ->bounded(
                min,
                max + 1
                );

}
