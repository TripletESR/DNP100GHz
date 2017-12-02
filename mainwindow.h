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
    SG6000LDQ,
    HMCT2220
};

enum multipler{
    x4 = 4,
    x6 = 6
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
    void on_actionSave_2_D_plot_triggered();

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
    void on_checkBox_Normalize_clicked(bool checked);

    void on_lineEdit_dBm_textChanged(const QString &arg1);
    void on_lineEdit_mW_textChanged(const QString &arg1);

    double dBm2mW(double dBm){
        return pow(10, dBm/10.);
    }
    double mW2dBm(double mW){
        return 10.* log(mW)/log(10.);
    }

    void on_lineEdit_Freq_returnPressed();

    void on_verticalSlider_power_valueChanged(int value);

private:
    Ui::MainWindow *ui;

    QCustomPlot *plot;
    QCustomPlot *auxPlot;
    QCPColorMap * colorMap;
    QCPColorScale * colorScale;

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

    QVector<double> * z; // for freq-power colorMap plot

    bool stopdBm2mW, stopmW2dBm;

};

#endif // MAINWINDOW_H
