#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QtTest>

#include "fibonacci_core.h"
#include "main_window.h"

class GuiSmokeTest : public QObject {
    Q_OBJECT

private slots:
    void windowBuilds();
    void runsSingleBenchmark();
};

void GuiSmokeTest::windowBuilds() {
    MainWindow window;
    window.show();

    QVERIFY(window.isVisible());
    QVERIFY(window.findChild<QPushButton*>("runButton") != nullptr);
    QVERIFY(window.findChild<QTableWidget*>("resultsTable") != nullptr);
}

void GuiSmokeTest::runsSingleBenchmark() {
    if (!fib::query_device().available) {
        QSKIP("CUDA device unavailable");
    }

    MainWindow window;
    window.show();

    auto* count = window.findChild<QSpinBox*>("countSpin");
    auto* run = window.findChild<QPushButton*>("runButton");
    auto* table = window.findChild<QTableWidget*>("resultsTable");
    QVERIFY(count != nullptr);
    QVERIFY(run != nullptr);
    QVERIFY(table != nullptr);

    count->setValue(5);
    QTest::mouseClick(run, Qt::LeftButton);

    QTRY_COMPARE_WITH_TIMEOUT(table->rowCount(), 1, 20000);
    QCOMPARE(table->item(0, 5)->text(), QStringLiteral("Yes"));
    QTRY_VERIFY_WITH_TIMEOUT(run->isEnabled(), 5000);
}

QTEST_MAIN(GuiSmokeTest)
#include "test_gui_smoke.moc"
