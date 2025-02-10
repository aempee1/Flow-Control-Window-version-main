#include "graph_dialog.hpp"
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QVBoxLayout>

GraphDialog::GraphDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Flow Data Graph");
    resize(800, 600);

    refSeries = new QLineSeries();
    refSeries->setName("Reference Flow");

    actSeries = new QLineSeries();
    actSeries->setName("Actual Flow");

    QChart* chart = new QChart();
    chart->addSeries(refSeries);
    chart->addSeries(actSeries);
    chart->createDefaultAxes();
    chart->setTitle("Flow Measurement Over Time");

    QValueAxis* axisX = new QValueAxis();
    axisX->setTitleText("Time (s)");
    axisX->setLabelFormat("%d");
    chart->setAxisX(axisX, refSeries);
    chart->setAxisX(axisX, actSeries);

    QValueAxis* axisY = new QValueAxis();
    axisY->setTitleText("Flow (m3/h)");
    axisY->setLabelFormat("%.2f");
    chart->setAxisY(axisY, refSeries);
    chart->setAxisY(axisY, actSeries);

    chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);

    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(chartView);
    setLayout(layout);
}

void GraphDialog::setData(const std::vector<double>& refFlow, const std::vector<double>& actFlow, const std::vector<long>& timestamps) {
    refSeries->clear();
    actSeries->clear();
    
    for (size_t i = 0; i < refFlow.size(); ++i) {
        refSeries->append(timestamps[i] / 1000.0, refFlow[i]);
        actSeries->append(timestamps[i] / 1000.0, actFlow[i]);
    }
}
