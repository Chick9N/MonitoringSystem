#include "minichartwidget.h"

#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

#include <QVBoxLayout>
#include <QLabel>
#include <QPainter>


MiniChartWidget::MiniChartWidget(
    ChartType type,
    QWidget *parent)
    : QWidget(parent)
    , m_type(type)
{
    setupChart();
}


void MiniChartWidget::setupChart()
{
    setMinimumSize(180, 100);

    QString titleText;

    if (m_type == ChartType::Temperature)
    {
        titleText = "温度 (°C)";
    }
    else
    {
        titleText = "电压 (V)";
    }

    QLabel *title = new QLabel(titleText, this);

    m_series = new QLineSeries();

    QChart *chart = new QChart();

    chart->addSeries(m_series);
    chart->legend()->hide();

    chart->setMargins(QMargins(2, 2, 2, 2));

    // X轴
    m_axisX = new QValueAxis();

    m_axisX->setRange(0, 30);
    m_axisX->setTickCount(4);
    m_axisX->setLabelFormat("%.0f");
    m_axisX->setGridLineVisible(false);

    // Y轴
    m_axisY = new QValueAxis();

    if (m_type == ChartType::Temperature)
    {
        m_axisY->setRange(15, 65);
    }
    else
    {
        m_axisY->setRange(20, 30);
    }

    m_axisY->setTickCount(3);
    m_axisY->setLabelFormat("%.1f");
    m_axisY->setGridLineVisible(false);

    chart->addAxis(
        m_axisX,
        Qt::AlignBottom
        );

    chart->addAxis(
        m_axisY,
        Qt::AlignLeft
        );

    m_series->attachAxis(m_axisX);
    m_series->attachAxis(m_axisY);

    m_chartView = new QChartView(chart);

    m_chartView->setRenderHint(
        QPainter::Antialiasing
        );

    QVBoxLayout *layout =
        new QVBoxLayout(this);

    layout->setContentsMargins(
        2, 2, 2, 2
        );

    layout->setSpacing(0);

    layout->addWidget(title);
    layout->addWidget(m_chartView);
}


void MiniChartWidget::setData(
    const QList<double> &data)
{
    m_series->clear();

    for (int i = 0; i < data.size(); ++i)
    {
        m_series->append(
            i,
            data.at(i)
            );
    }

    updateYAxis(data);

    m_axisX->setRange(0, 30);
}


void MiniChartWidget::updateYAxis(
    const QList<double> &data)
{
    if (data.isEmpty())
        return;

    double minValue = data.first();
    double maxValue = data.first();

    for (double value : data)
    {
        if (value < minValue)
            minValue = value;

        if (value > maxValue)
            maxValue = value;
    }

    double padding;

    if (m_type == ChartType::Temperature)
    {
        padding = 2.0;
    }
    else
    {
        padding = 0.2;
    }

    double lower = minValue - padding;
    double upper = maxValue + padding;

    // 防止上下范围太接近
    if (upper - lower < 0.5)
    {
        double center =
            (upper + lower) / 2.0;

        lower = center - 0.25;
        upper = center + 0.25;
    }

    m_axisY->setRange(
        lower,
        upper
        );
}
