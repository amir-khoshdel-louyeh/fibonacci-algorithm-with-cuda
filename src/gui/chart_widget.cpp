#include "chart_widget.h"

#include <algorithm>
#include <cmath>

#include <QAbstractAxis>
#include <QChart>
#include <QChartView>
#include <QLineSeries>
#include <QLogValueAxis>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QValueAxis>
#include <QVBoxLayout>

ChartWidget::ChartWidget(QWidget* parent) : QWidget(parent) {
    chart_ = new QChart();
    chart_->setAnimationOptions(QChart::SeriesAnimations);
    chart_->setBackgroundRoundness(0);
    chart_->legend()->setAlignment(Qt::AlignBottom);

    view_ = new QChartView(chart_, this);
    view_->setObjectName(QStringLiteral("chartView"));
    view_->setRenderHint(QPainter::Antialiasing);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(view_);

    setMinimumHeight(280);

    rebuild();
}

void ChartWidget::clear() {
    results_.clear();
    rebuild();
}

void ChartWidget::addResult(const fib::BenchmarkResult& result) {
    results_.push_back(result);
    rebuild();
}

void ChartWidget::setMode(Mode mode) {
    if (mode_ == mode) {
        return;
    }
    mode_ = mode;
    rebuild();
}

void ChartWidget::setLogScale(bool enabled) {
    if (log_scale_ == enabled) {
        return;
    }
    log_scale_ = enabled;
    rebuild();
}

bool ChartWidget::savePng(const QString& path, QString& error) const {
    if (results_.empty()) {
        error = tr("There is no chart data to export.");
        return false;
    }

    const QPixmap pixmap = view_->grab();
    if (pixmap.isNull() || !pixmap.save(path, "PNG")) {
        error = tr("Could not write the chart image to %1.").arg(path);
        return false;
    }
    return true;
}

void ChartWidget::rebuild() {
    chart_->removeAllSeries();
    const QList<QAbstractAxis*> axes = chart_->axes();
    for (QAbstractAxis* axis : axes) {
        chart_->removeAxis(axis);
        delete axis;
    }

    if (results_.empty()) {
        chart_->setTitle(tr("Run a benchmark to see results"));
        return;
    }

    double maximum_seconds = 0.0;
    double minimum_seconds = 0.0;
    for (const fib::BenchmarkResult& result : results_) {
        maximum_seconds = std::max(maximum_seconds, result.cpu_seconds);
        maximum_seconds = std::max(maximum_seconds, result.gpu_seconds);
        if (result.cpu_seconds > 0.0) {
            minimum_seconds = minimum_seconds == 0.0
                                  ? result.cpu_seconds
                                  : std::min(minimum_seconds, result.cpu_seconds);
        }
        if (result.gpu_seconds > 0.0) {
            minimum_seconds = minimum_seconds == 0.0
                                  ? result.gpu_seconds
                                  : std::min(minimum_seconds, result.gpu_seconds);
        }
    }
    minimum_seconds = std::max(minimum_seconds, 1e-12);

    const int first_count = results_.front().count;
    const int last_count = results_.back().count;
    auto* x_axis = new QValueAxis();
    x_axis->setTitleText(tr("Fibonacci terms"));
    x_axis->setLabelFormat(QStringLiteral("%.0f"));
    if (first_count == last_count) {
        x_axis->setRange(first_count - 1.0, last_count + 1.0);
        x_axis->setTickCount(2);
    } else {
        x_axis->setRange(first_count - 0.5, last_count + 0.5);
        x_axis->setTickCount(
            static_cast<int>(std::clamp<std::size_t>(results_.size(), 2, 8)));
    }
    chart_->addAxis(x_axis, Qt::AlignBottom);

    const bool show_points = results_.size() <= 25;

    if (mode_ == Mode::Timings) {
        auto* cpu_series = new QLineSeries();
        cpu_series->setName(tr("CPU"));
        cpu_series->setColor(QColor("#d9534f"));
        cpu_series->setPointsVisible(show_points);

        auto* gpu_series = new QLineSeries();
        gpu_series->setName(tr("GPU (end to end)"));
        gpu_series->setColor(QColor("#2e8b57"));
        gpu_series->setPointsVisible(show_points);

        for (const fib::BenchmarkResult& result : results_) {
            cpu_series->append(result.count, result.cpu_seconds);
            gpu_series->append(result.count, result.gpu_seconds);
        }

        QAbstractAxis* y_axis = nullptr;
        if (log_scale_ && maximum_seconds > 0.0) {
            auto* log_axis = new QLogValueAxis();
            log_axis->setTitleText(tr("Time (s, logarithmic)"));
            log_axis->setBase(10.0);
            log_axis->setLabelFormat(QStringLiteral("%.1e"));
            log_axis->setRange(minimum_seconds * 0.5,
                               std::max(maximum_seconds * 2.0,
                                        minimum_seconds * 1.5));
            y_axis = log_axis;
        } else {
            auto* linear_axis = new QValueAxis();
            linear_axis->setTitleText(tr("Time (s)"));
            linear_axis->setLabelFormat(QStringLiteral("%.2e"));
            linear_axis->setRange(
                0.0, maximum_seconds > 0.0 ? maximum_seconds * 1.1 : 1.0);
            linear_axis->applyNiceNumbers();
            y_axis = linear_axis;
        }

        chart_->addSeries(cpu_series);
        chart_->addSeries(gpu_series);
        chart_->addAxis(y_axis, Qt::AlignLeft);
        for (QLineSeries* series : {cpu_series, gpu_series}) {
            series->attachAxis(x_axis);
            series->attachAxis(y_axis);
        }
        chart_->setTitle(tr("Execution time by term count"));
        return;
    }

    auto* speedup_series = new QLineSeries();
    speedup_series->setName(tr("CPU / GPU"));
    speedup_series->setColor(QColor("#2f6fb0"));
    speedup_series->setPointsVisible(show_points);

    double maximum_speedup = 0.0;
    for (const fib::BenchmarkResult& result : results_) {
        const double speedup =
            result.gpu_seconds > 0.0
                ? result.cpu_seconds / result.gpu_seconds
                : 0.0;
        speedup_series->append(result.count, speedup);
        maximum_speedup = std::max(maximum_speedup, speedup);
    }

    auto* break_even = new QLineSeries();
    break_even->setName(tr("Break-even"));
    break_even->setColor(Qt::gray);
    QPen dashed = break_even->pen();
    dashed.setStyle(Qt::DashLine);
    break_even->setPen(dashed);
    break_even->append(first_count - 0.5, 1.0);
    break_even->append(last_count + 0.5, 1.0);

    auto* y_axis = new QValueAxis();
    y_axis->setTitleText(tr("Speedup"));
    y_axis->setLabelFormat(QStringLiteral("%.1f"));
    y_axis->setRange(0.0, std::max(1.2, maximum_speedup * 1.15));
    y_axis->applyNiceNumbers();

    chart_->addSeries(speedup_series);
    chart_->addSeries(break_even);
    chart_->addAxis(y_axis, Qt::AlignLeft);
    for (QLineSeries* series : {speedup_series, break_even}) {
        series->attachAxis(x_axis);
        series->attachAxis(y_axis);
    }
    chart_->setTitle(tr("CPU / GPU speedup (above 1 means GPU is faster)"));
}
