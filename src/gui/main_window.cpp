#include "main_window.h"

#include <QLabel>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(tr("Fibonacci CUDA Benchmark"));

    auto* placeholder = new QLabel(tr("Benchmark controls will appear here."), this);
    placeholder->setAlignment(Qt::AlignCenter);
    setCentralWidget(placeholder);

    resize(1000, 700);
}
