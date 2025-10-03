#pragma once

#include <vector>

#include <QWidget>

#include "fibonacci_core.h"

class QChart;
class QChartView;
class QString;

class ChartWidget : public QWidget {
    Q_OBJECT

public:
    enum class Mode {
        Timings,
        Speedup,
    };

    explicit ChartWidget(QWidget* parent = nullptr);

    void clear();
    void addResult(const fib::BenchmarkResult& result);
    void setMode(Mode mode);
    void setLogScale(bool enabled);
    bool savePng(const QString& path, QString& error) const;

private:
    void rebuild();

    QChart* chart_ = nullptr;
    QChartView* view_ = nullptr;
    std::vector<fib::BenchmarkResult> results_;
    Mode mode_ = Mode::Timings;
    bool log_scale_ = false;
};
