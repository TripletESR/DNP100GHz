#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtSerialPort>
#include "qcustomplot.h"
#include "qscpi.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void LogMsg(QString str);
    void on_pushButton_RFonoff_clicked();

    void findSeriesPortDevices();
    void write2Device(const QString &msg);
    QString readFromDevice();

private:
    Ui::MainWindow *ui;

    QCustomPlot *plot;

    QSCPI * powerMeter;
    QSerialPort * generator;

    bool RFOnOff;
};

#endif // MAINWINDOW_H
