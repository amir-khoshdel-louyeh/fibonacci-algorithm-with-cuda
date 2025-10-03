#pragma once

#include <atomic>

#include <QMetaType>
#include <QObject>
#include <QString>

#include "fibonacci_core.h"

Q_DECLARE_METATYPE(fib::BenchmarkResult)

class BenchmarkWorker : public QObject {
    Q_OBJECT

public:
    explicit BenchmarkWorker(QObject* parent = nullptr);

    void requestCancel();

public slots:
    void run(int first_count, int last_count, fib::BenchmarkOptions options);

signals:
    void progress(int completed, int total);
    void resultReady(const fib::BenchmarkResult& result);
    void finished(const QString& error, bool canceled);

private:
    std::atomic_bool cancel_requested_{false};
};
