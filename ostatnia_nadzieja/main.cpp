#include <QApplication>
#include <QDialog>
#include <QFormLayout>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include "mainwindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    // Simple input dialog
    QDialog dlg;
    dlg.setWindowTitle("Symulator połączeń - konfiguracja");
    auto* layout = new QFormLayout(&dlg);

    auto* colsSpin  = new QSpinBox; colsSpin->setRange(2, 50); colsSpin->setValue(7);
    auto* rowsSpin  = new QSpinBox; rowsSpin->setRange(2, 50); rowsSpin->setValue(7);
    auto* goalSpin  = new QSpinBox; goalSpin->setRange(1, 50); goalSpin->setValue(8);
    auto* startBtn  = new QPushButton("Start");

    layout->addRow("Kolumny:", colsSpin);
    layout->addRow("Rzędy:",    rowsSpin);
    layout->addRow("Liczba celów:", goalSpin);
    layout->addRow(startBtn);

    QObject::connect(startBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    if (dlg.exec() != QDialog::Accepted) return 0;

    int cols  = colsSpin->value();
    int rows  = rowsSpin->value();
    int goals = std::min(goalSpin->value(), cols * rows - 1);

    MainWindow w(cols, rows, goals);
    w.resize(640, 480);
    w.show();
    return app.exec();
}