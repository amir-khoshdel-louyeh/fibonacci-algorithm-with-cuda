#include "benchmark_worker.h"

#include <exception>

BenchmarkWorker::BenchmarkWorker(QObject* parent) : QObject(parent) {}

void BenchmarkWorker::requestCancel() {
    cancel_requested_.store(true);
}

void BenchmarkWorker::run(int first_count, int last_count,
                          fib::BenchmarkOptions options) {
    cancel_requested_.store(false);

    if (first_count < 1 || last_count < first_count ||
        last_count > fib::kMaxCount) {
        emit finished(tr("Invalid benchmark range."), false);
        return;
    }

    const int total = last_count - first_count + 1;
    try {
        for (int count = first_count; count <= last_count; ++count) {
            if (cancel_requested_.load()) {
                emit finished(QString(), true);
                return;
            }

            const fib::BenchmarkResult result =
                fib::run_benchmark(count, options);
            emit resultReady(result);
            emit progress(count - first_count + 1, total);

            if (!result.results_match) {
                emit finished(tr("CPU and GPU results differ for %1 terms.")
                                  .arg(count),
                              false);
                return;
            }
        }
    } catch (const std::exception& error) {
        emit finished(QString::fromUtf8(error.what()), false);
        return;
    }

    emit finished(QString(), false);
}
