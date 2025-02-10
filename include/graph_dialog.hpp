#ifndef GRAPH_DIALOG_HPP
#define GRAPH_DIALOG_HPP

#include <QDialog>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QChart>
#include <QVBoxLayout>

QT_CHARTS_USE_NAMESPACE

class GraphDialog : public QDialog {
    Q_OBJECT

public:
    GraphDialog(QWidget* parent = nullptr);
    void setData(const std::vector<double>& refFlow, const std::vector<double>& actFlow, const std::vector<long>& timestamps);

private:
    QChartView* chartView;
    QLineSeries* refSeries;
    QLineSeries* actSeries;
};

#endif // GRAPH_DIALOG_HPP
