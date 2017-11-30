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

enum generator_Type{
    smallGenerator,
    HMCT2220
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void LogMsg(QString str, QColor color = Qt::black);
    void on_pushButton_Sweep_clicked();

    void findSeriesPortDevices();
    void write2Generator(const QString &msg);
    void readFromGenerator();

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

    void on_lineEdit_PowerStart_textChanged(const QString &arg1);

    void on_lineEdit_PowerEnd_textChanged(const QString &arg1);

    void on_spinBox_PowerStep_valueChanged(int arg1);

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
    int generatorCount;

    bool rfOnOff;
    bool sweepOnOff;
    int msgCount;
    bool switchConnected;

    bool hasPowerMeter, hasDMM;

    int generatorType;

    int programMode; // 1 = smallGenerator + PM/DMM, fixed power, simple measurement
                     // 2 = any Generator + PM + DM, fixed power, Calibration
                     // 3 = Hittite + PM/DMM, vary power, simple measurement.
                     // if two generaotrs and PM and DMM are connected, it is mode 2 using Hittite

    QVector<double> x, y, y2, dB, dB2;
};

#endif // MAINWINDOW_H
