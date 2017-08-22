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
    void on_pushButton_Sweep_clicked();

    void findSeriesPortDevices();
    void write2Device(const QString &msg);
    void readFromDevice();

    void controlOnOFF(bool IO);

    void on_pushButton_SendCommand_clicked();

    void on_lineEdit_Points_textChanged(const QString &arg1);

    void on_lineEdit_Dwell_textChanged(const QString &arg1);

    void on_lineEdit_Start_textChanged(const QString &arg1);

    void on_lineEdit_Stop_textChanged(const QString &arg1);

    void on_doubleSpinBox_Power_valueChanged(double arg1);

    void on_doubleSpinBox_valueChanged(double arg1);

    void on_actionSave_Data_triggered();

    void on_pushButton_ReadPower_clicked();

    void on_actionSave_plot_triggered();

private:
    Ui::MainWindow *ui;

    QCustomPlot *plot;

    QSCPI * powerMeter;
    QSerialPort * generator;

    QString generatorPortName;
    QString generatorLastRepond;

    bool RFOnOff;
    int msgCount;

    QVector<double> x, y;
};

#endif // MAINWINDOW_H
