#pragma once

#include <vector>

#include <QMainWindow>
#include <QPointer>

#include "fibonacci_core.h"

class BenchmarkWorker;
class QComboBox;
class QLabel;
class QProgressBar;
class QPushButton;
class QSpinBox;
class QTableWidget;
class QThread;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void startBenchmark();
    void cancelBenchmark();
    void updateControlState();
    void onProgress(int completed, int total);
    void onResultReady(const fib::BenchmarkResult& result);
    void onFinished(const QString& error, bool canceled);

private:
    void buildUi();
    void showDeviceInfo();
    void setRunning(bool running);
    void appendResult(const fib::BenchmarkResult& result);

    QComboBox* mode_combo_ = nullptr;
    QSpinBox* count_spin_ = nullptr;
    QSpinBox* sweep_start_spin_ = nullptr;
    QSpinBox* sweep_end_spin_ = nullptr;
    QSpinBox* iterations_spin_ = nullptr;
    QSpinBox* warmup_spin_ = nullptr;
    QPushButton* run_button_ = nullptr;
    QPushButton* cancel_button_ = nullptr;
    QProgressBar* progress_bar_ = nullptr;
    QLabel* device_label_ = nullptr;
    QTableWidget* results_table_ = nullptr;

    std::vector<fib::BenchmarkResult> results_;
    fib::DeviceInfo device_;
    QPointer<QThread> thread_;
    QPointer<BenchmarkWorker> worker_;
    bool running_ = false;
};
