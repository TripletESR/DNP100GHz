#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtSerialPort>
#include <QAxObject>
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
    void LogMsg(QString str, bool warningFlag = false);
    void on_pushButton_Sweep_clicked();

    void findSeriesPortDevices();
    void write2SmallGenerator(const QString &msg);
    void readFromDevice();

    void SetSwitchMatrixPort(QString slot, int port);
    void controlOnOFF(bool IO);

    void on_pushButton_SendCommand_clicked();
    void on_spinBox_Points_valueChanged(int arg1);
    void on_spinBox_Dwell_valueChanged(int arg1);
    void on_lineEdit_Start_textChanged(const QString &arg1);
    void on_lineEdit_Stop_textChanged(const QString &arg1);
    void on_doubleSpinBox_Power_valueChanged(double arg1);
    void on_doubleSpinBox_valueChanged(double arg1);

    void on_actionSave_Data_triggered();
    void on_actionSave_plot_triggered();

    void on_pushButton_ReadPower_clicked();
    void on_pushButton_RFOnOff_clicked();

    void on_lineEdit_Freq_textChanged(const QString &arg1);
    void on_lineEdit_Multiplier_textChanged(const QString &arg1);
    
    void on_comboBox_yAxis_currentIndexChanged(int index);
    void on_spinBox_Average_valueChanged(int arg1);

    void on_horizontalSlider_A_valueChanged(int value);
    void on_horizontalSlider_B_valueChanged(int value);

    void checkPowerMeterFreq(double freq);

    void on_pushButton_GetNoPoint_clicked();
    void on_spinBox_AveragePoints_editingFinished();

private:
    Ui::MainWindow *ui;

    QCustomPlot *plot;
    QCustomPlot *comparePlot;

    QSCPI * powerMeter;
    QSCPI * DMM;
    QSerialPort * generator;
    QAxObject * switchMatrix;

    QString generatorPortName;
    QString generatorLastRepond;

    bool rfOnOff;
    bool sweepOnOff;
    int msgCount;
    bool switchConnected;

    bool hasPowerMeter, hasDMM;

    QVector<double> x, y, y2, dB, dB2;
};

#endif // MAINWINDOW_H
