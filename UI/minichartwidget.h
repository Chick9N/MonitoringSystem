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

    QChartView *m_chartView = nullptr;
    QLineSeries *m_series = nullptr;

    QValueAxis *m_axisX = nullptr;
    QValueAxis *m_axisY = nullptr;
};

#endif // MINICHARTWIDGET_H
