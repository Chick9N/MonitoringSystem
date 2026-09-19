#ifndef MINICHARTWIDGET_H
#define MINICHARTWIDGET_H

#include <QWidget>
#include <QList>
#include <QString>

class QChartView;
class QLineSeries;
class QValueAxis;

enum class ChartType
{
    Temperature,
    Voltage
};

class MiniChartWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MiniChartWidget(
        ChartType type,
        QWidget *parent = nullptr
        );

    void setData(const QList<double> &data);

private:
    void setupChart();
    void updateYAxis(const QList<double> &data);

private:
    ChartType m_type;

    QChartView *m_chartView;
    QLineSeries *m_series;

    QValueAxis *m_axisX;
    QValueAxis *m_axisY;
};

#endif // MINICHARTWIDGET_H
