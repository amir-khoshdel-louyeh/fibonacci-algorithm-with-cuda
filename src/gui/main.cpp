#include <QApplication>
#include <QMetaType>

#include "benchmark_worker.h"
#include "main_window.h"

int main(int argc, char* argv[]) {
    QApplication application(argc, argv);
    qRegisterMetaType<fib::BenchmarkResult>("fib::BenchmarkResult");
    application.setApplicationName("Fibonacci CUDA Benchmark");
    application.setApplicationDisplayName("Fibonacci CUDA Benchmark");
    application.setApplicationVersion("0.1.0");
    application.setOrganizationName("Fibonacci CUDA");

    MainWindow window;
    window.show();
    return application.exec();
}
