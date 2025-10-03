#include "main_window.h"

#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QSplitter>
#include <QStatusBar>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QThread>
#include <QVBoxLayout>
#include <QWidget>

#include "benchmark_io.h"
#include "benchmark_worker.h"
#include "chart_widget.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    buildUi();
    showDeviceInfo();
    updateControlState();
}

MainWindow::~MainWindow() {
    if (worker_) {
        worker_->requestCancel();
    }
    if (thread_) {
        thread_->quit();
        thread_->wait();
        delete thread_;
    }
    delete worker_;
}

void MainWindow::buildUi() {
    setWindowTitle(tr("Fibonacci CUDA Benchmark"));

    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(10);

    auto* controls = new QGroupBox(tr("Benchmark settings"), central);
    auto* controls_layout = new QHBoxLayout(controls);

    auto make_spin = [controls](int minimum, int maximum, int value) {
        auto* spin = new QSpinBox(controls);
        spin->setRange(minimum, maximum);
        spin->setValue(value);
        return spin;
    };

    mode_combo_ = new QComboBox(controls);
    mode_combo_->setObjectName(QStringLiteral("modeCombo"));
    mode_combo_->addItems({tr("Single count"), tr("Sweep range")});
    count_spin_ = make_spin(1, fib::kMaxCount, 30);
    count_spin_->setObjectName(QStringLiteral("countSpin"));
    sweep_start_spin_ = make_spin(1, fib::kMaxCount, 5);
    sweep_start_spin_->setObjectName(QStringLiteral("sweepStartSpin"));
    sweep_end_spin_ = make_spin(1, fib::kMaxCount, 20);
    sweep_end_spin_->setObjectName(QStringLiteral("sweepEndSpin"));
    iterations_spin_ = make_spin(1, 100, 3);
    iterations_spin_->setObjectName(QStringLiteral("iterationsSpin"));
    warmup_spin_ = make_spin(0, 100, 1);
    warmup_spin_->setObjectName(QStringLiteral("warmupSpin"));

    auto* sweep_widget = new QWidget(controls);
    auto* sweep_layout = new QHBoxLayout(sweep_widget);
    sweep_layout->setContentsMargins(0, 0, 0, 0);
    sweep_layout->addWidget(sweep_start_spin_);
    sweep_layout->addWidget(new QLabel(tr("to"), sweep_widget));
    sweep_layout->addWidget(sweep_end_spin_);

    auto* form = new QFormLayout();
    form->addRow(tr("Mode"), mode_combo_);
    form->addRow(tr("Count"), count_spin_);
    form->addRow(tr("Sweep"), sweep_widget);
    form->addRow(tr("Iterations"), iterations_spin_);
    form->addRow(tr("Warmup"), warmup_spin_);
    controls_layout->addLayout(form, 1);

    auto* actions = new QVBoxLayout();
    run_button_ = new QPushButton(tr("Run benchmark"), controls);
    run_button_->setObjectName(QStringLiteral("runButton"));
    run_button_->setDefault(true);
    cancel_button_ = new QPushButton(tr("Cancel"), controls);
    cancel_button_->setObjectName(QStringLiteral("cancelButton"));
    cancel_button_->setEnabled(false);
    progress_bar_ = new QProgressBar(controls);
    progress_bar_->setObjectName(QStringLiteral("progressBar"));
    progress_bar_->setRange(0, 1);
    progress_bar_->setValue(0);
    progress_bar_->setVisible(false);
    actions->addWidget(run_button_);
    actions->addWidget(cancel_button_);
    actions->addWidget(progress_bar_);
    actions->addStretch();
    controls_layout->addLayout(actions);

    layout->addWidget(controls);

    auto* results_group = new QGroupBox(tr("Results"), central);
    auto* results_layout = new QVBoxLayout(results_group);
    results_table_ = new QTableWidget(0, 6, results_group);
    results_table_->setObjectName(QStringLiteral("resultsTable"));
    results_table_->setHorizontalHeaderLabels(
        {tr("Count"), tr("CPU (s)"), tr("GPU (s)"), tr("Kernel (s)"),
         tr("CPU / GPU"), tr("Match")});
    results_table_->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Stretch);
    results_table_->verticalHeader()->setVisible(false);
    results_table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    results_table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    results_table_->setAlternatingRowColors(true);
    results_layout->addWidget(results_table_);

    auto* chart_group = new QGroupBox(tr("Visualization"), central);
    auto* chart_layout = new QVBoxLayout(chart_group);
    auto* chart_controls = new QHBoxLayout();
    chart_mode_combo_ = new QComboBox(chart_group);
    chart_mode_combo_->setObjectName(QStringLiteral("chartModeCombo"));
    chart_mode_combo_->addItems(
        {tr("Execution time"), tr("CPU / GPU speedup")});
    log_scale_check_ = new QCheckBox(tr("Logarithmic scale"), chart_group);
    log_scale_check_->setObjectName(QStringLiteral("logScaleCheck"));
    chart_controls->addWidget(new QLabel(tr("Chart"), chart_group));
    chart_controls->addWidget(chart_mode_combo_);
    chart_controls->addStretch();
    chart_controls->addWidget(log_scale_check_);
    chart_layout->addLayout(chart_controls);

    chart_ = new ChartWidget(chart_group);
    chart_->setObjectName(QStringLiteral("chartWidget"));
    chart_layout->addWidget(chart_, 1);

    auto* splitter = new QSplitter(Qt::Vertical, central);
    splitter->addWidget(results_group);
    splitter->addWidget(chart_group);
    splitter->setChildrenCollapsible(false);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    splitter->setSizes({240, 360});
    layout->addWidget(splitter, 1);

    setCentralWidget(central);

    device_label_ = new QLabel(this);
    statusBar()->addPermanentWidget(device_label_);
    statusBar()->showMessage(tr("Ready."));

    QMenu* file_menu = menuBar()->addMenu(tr("&File"));
    open_csv_action_ = file_menu->addAction(tr("&Open session..."), this,
                                            &MainWindow::openCsvSession);
    file_menu->addSeparator();
    save_csv_action_ = file_menu->addAction(tr("Save &CSV..."), this,
                                            &MainWindow::saveCsv);
    save_json_action_ = file_menu->addAction(tr("Save &JSON..."), this,
                                             &MainWindow::saveJson);
    export_image_action_ = file_menu->addAction(
        tr("Export chart &image..."), this, &MainWindow::exportChart);
    file_menu->addSeparator();
    file_menu->addAction(tr("&Quit"), this, &QWidget::close);

    QMenu* help_menu = menuBar()->addMenu(tr("&Help"));
    help_menu->addAction(tr("&About"), this, &MainWindow::showAbout);

    connect(mode_combo_, &QComboBox::currentIndexChanged, this,
            &MainWindow::updateControlState);
    connect(run_button_, &QPushButton::clicked, this,
            &MainWindow::startBenchmark);
    connect(cancel_button_, &QPushButton::clicked, this,
            &MainWindow::cancelBenchmark);
    connect(sweep_start_spin_, &QSpinBox::valueChanged, this,
            [this](int value) {
                if (sweep_end_spin_->value() < value) {
                    sweep_end_spin_->setValue(value);
                }
            });
    connect(chart_mode_combo_, &QComboBox::currentIndexChanged, this,
            [this](int index) {
                chart_->setMode(index == 0 ? ChartWidget::Mode::Timings
                                           : ChartWidget::Mode::Speedup);
                log_scale_check_->setEnabled(index == 0);
            });
    connect(log_scale_check_, &QCheckBox::toggled, this,
            [this](bool checked) { chart_->setLogScale(checked); });

    resize(1100, 800);
}

void MainWindow::showDeviceInfo() {
    device_ = fib::query_device();
    if (device_.available) {
        const double gibibytes =
            static_cast<double>(device_.total_memory_bytes) /
            (1024.0 * 1024.0 * 1024.0);
        device_label_->setText(
            tr("%1 · sm_%2%3 · %4 GiB · %5 SMs")
                .arg(QString::fromStdString(device_.name))
                .arg(device_.compute_major)
                .arg(device_.compute_minor)
                .arg(QString::number(gibibytes, 'f', 1))
                .arg(device_.multiprocessor_count));
    } else {
        device_label_->setText(tr("No CUDA device"));
        statusBar()->showMessage(
            tr("CUDA unavailable: %1")
                .arg(QString::fromStdString(device_.error)));
    }
}

void MainWindow::updateControlState() {
    const bool single = mode_combo_->currentIndex() == 0;
    mode_combo_->setEnabled(!running_);
    count_spin_->setEnabled(!running_ && single);
    sweep_start_spin_->setEnabled(!running_ && !single);
    sweep_end_spin_->setEnabled(!running_ && !single);
    iterations_spin_->setEnabled(!running_);
    warmup_spin_->setEnabled(!running_);
    run_button_->setEnabled(!running_ && device_.available);
    cancel_button_->setEnabled(running_);
    updateActions();
}

void MainWindow::updateActions() {
    const bool has_results = !results_.empty();
    if (open_csv_action_ != nullptr) {
        open_csv_action_->setEnabled(!running_);
    }
    if (save_csv_action_ != nullptr) {
        save_csv_action_->setEnabled(has_results && !running_);
    }
    if (save_json_action_ != nullptr) {
        save_json_action_->setEnabled(has_results && !running_);
    }
    if (export_image_action_ != nullptr) {
        export_image_action_->setEnabled(has_results && !running_);
    }
}

void MainWindow::setRunning(bool running) {
    running_ = running;
    updateControlState();
}

void MainWindow::startBenchmark() {
    if (running_) {
        return;
    }

    const bool single = mode_combo_->currentIndex() == 0;
    const int first_count =
        single ? count_spin_->value() : sweep_start_spin_->value();
    const int last_count =
        single ? first_count : sweep_end_spin_->value();
    if (last_count < first_count) {
        QMessageBox::warning(this, tr("Invalid range"),
                             tr("The sweep start must not exceed the end."));
        return;
    }

    fib::BenchmarkOptions options;
    options.iterations = iterations_spin_->value();
    options.warmup_iterations = warmup_spin_->value();

    results_.clear();
    results_table_->setRowCount(0);
    chart_->clear();
    progress_bar_->setRange(0, last_count - first_count + 1);
    progress_bar_->setValue(0);
    progress_bar_->setVisible(true);
    statusBar()->showMessage(tr("Running benchmark..."));

    auto* thread = new QThread();
    auto* worker = new BenchmarkWorker();
    worker->moveToThread(thread);

    connect(thread, &QThread::started, worker,
            [worker, first_count, last_count, options] {
                worker->run(first_count, last_count, options);
            });
    connect(worker, &BenchmarkWorker::progress, this,
            &MainWindow::onProgress);
    connect(worker, &BenchmarkWorker::resultReady, this,
            &MainWindow::onResultReady);
    connect(worker, &BenchmarkWorker::finished, this,
            &MainWindow::onFinished);
    connect(worker, &BenchmarkWorker::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, worker, &QObject::deleteLater);
    connect(thread, &QThread::finished, this, [this, thread] {
        if (thread_ == thread) {
            thread_ = nullptr;
            worker_ = nullptr;
        }
        thread->deleteLater();
    });

    thread_ = thread;
    worker_ = worker;
    setRunning(true);
    thread->start();
}

void MainWindow::cancelBenchmark() {
    if (!worker_) {
        return;
    }
    worker_->requestCancel();
    cancel_button_->setEnabled(false);
    statusBar()->showMessage(tr("Canceling..."));
}

void MainWindow::onProgress(int completed, int total) {
    progress_bar_->setMaximum(total);
    progress_bar_->setValue(completed);
    statusBar()->showMessage(
        tr("Completed %1 of %2 term set(s)...").arg(completed).arg(total));
}

void MainWindow::onResultReady(const fib::BenchmarkResult& result) {
    appendResult(result);
}

void MainWindow::appendResult(const fib::BenchmarkResult& result) {
    results_.push_back(result);
    chart_->addResult(result);

    const int row = results_table_->rowCount();
    results_table_->insertRow(row);

    auto set_number = [this, row](int column, const QString& text) {
        auto* item = new QTableWidgetItem(text);
        item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        results_table_->setItem(row, column, item);
    };

    set_number(0, QString::number(result.count));
    set_number(1, QString::number(result.cpu_seconds, 'g', 9));
    set_number(2, QString::number(result.gpu_seconds, 'g', 9));
    set_number(3, QString::number(result.gpu_kernel_seconds, 'g', 9));
    if (result.gpu_seconds > 0.0) {
        set_number(4, QString::number(result.cpu_seconds / result.gpu_seconds,
                                      'f', 2) +
                          QStringLiteral("x"));
    } else {
        set_number(4, QStringLiteral("-"));
    }

    auto* match = new QTableWidgetItem(result.results_match ? tr("Yes")
                                                            : tr("No"));
    match->setTextAlignment(Qt::AlignCenter);
    if (!result.results_match) {
        match->setForeground(Qt::red);
    }
    results_table_->setItem(row, 5, match);
    results_table_->scrollToBottom();
    updateActions();
}

void MainWindow::openCsvSession() {
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Open benchmark session"), QString(),
        tr("CSV files (*.csv);;All files (*)"));
    if (path.isEmpty()) {
        return;
    }

    std::vector<fib::BenchmarkResult> loaded;
    std::string error;
    if (!fib::read_csv(path.toStdString(), loaded, error)) {
        QMessageBox::warning(this, tr("Open failed"),
                             QString::fromStdString(error));
        return;
    }
    if (loaded.empty()) {
        QMessageBox::information(
            this, tr("Open session"),
            tr("The selected file contains no benchmark results."));
        return;
    }

    results_.clear();
    results_table_->setRowCount(0);
    chart_->clear();
    for (const fib::BenchmarkResult& result : loaded) {
        appendResult(result);
    }
    statusBar()->showMessage(
        tr("Loaded %1 result(s) from %2")
            .arg(static_cast<int>(loaded.size()))
            .arg(path));
}

void MainWindow::saveCsv() {
    if (results_.empty()) {
        return;
    }
    QString path = QFileDialog::getSaveFileName(
        this, tr("Save benchmark results"),
        QStringLiteral("fibonacci-benchmark.csv"), tr("CSV files (*.csv)"));
    if (path.isEmpty()) {
        return;
    }
    if (QFileInfo(path).suffix().isEmpty()) {
        path += QStringLiteral(".csv");
    }

    std::string error;
    if (!fib::write_csv(path.toStdString(), results_, error)) {
        QMessageBox::warning(this, tr("Save failed"),
                             QString::fromStdString(error));
        return;
    }
    statusBar()->showMessage(
        tr("Saved %1 result(s) to %2")
            .arg(static_cast<int>(results_.size()))
            .arg(path));
}

void MainWindow::saveJson() {
    if (results_.empty()) {
        return;
    }
    QString path = QFileDialog::getSaveFileName(
        this, tr("Save benchmark results"),
        QStringLiteral("fibonacci-benchmark.json"), tr("JSON files (*.json)"));
    if (path.isEmpty()) {
        return;
    }
    if (QFileInfo(path).suffix().isEmpty()) {
        path += QStringLiteral(".json");
    }

    std::string error;
    if (!fib::write_json(path.toStdString(), results_, error)) {
        QMessageBox::warning(this, tr("Save failed"),
                             QString::fromStdString(error));
        return;
    }
    statusBar()->showMessage(
        tr("Saved %1 result(s) to %2")
            .arg(static_cast<int>(results_.size()))
            .arg(path));
}

void MainWindow::exportChart() {
    if (results_.empty()) {
        return;
    }
    QString path = QFileDialog::getSaveFileName(
        this, tr("Export chart image"),
        QStringLiteral("fibonacci-chart.png"), tr("PNG images (*.png)"));
    if (path.isEmpty()) {
        return;
    }
    if (QFileInfo(path).suffix().isEmpty()) {
        path += QStringLiteral(".png");
    }

    QString error;
    if (!chart_->savePng(path, error)) {
        QMessageBox::warning(this, tr("Export failed"), error);
        return;
    }
    statusBar()->showMessage(tr("Saved chart image to %1").arg(path));
}

void MainWindow::showAbout() {
    QMessageBox::about(
        this, tr("About Fibonacci CUDA Benchmark"),
        tr("<h3>Fibonacci CUDA Benchmark</h3>"
           "<p>Compares a recursive Fibonacci implementation on the CPU with "
           "a single-threaded CUDA kernel.</p>"
           "<p>GPU timings include allocation, kernel execution, and transfer "
           "overhead; kernel time is measured with CUDA events.</p>"
           "<p>Author: Amir Khoshdel Louyeh</p>"));
}

void MainWindow::onFinished(const QString& error, bool canceled) {
    progress_bar_->setVisible(false);
    setRunning(false);

    if (!error.isEmpty()) {
        statusBar()->showMessage(error);
        QMessageBox::warning(this, tr("Benchmark failed"), error);
    } else if (canceled) {
        statusBar()->showMessage(tr("Benchmark canceled."));
    } else {
        statusBar()->showMessage(tr("Benchmark complete."));
    }
}
