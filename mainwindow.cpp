#include "mainwindow.h"
#include "ui_mainwindow.h"

const QString DESKTOP_PATH = QStandardPaths::locate(QStandardPaths::DesktopLocation, QString(), QStandardPaths::LocateDirectory);

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    RFOnOff = false;
    msgCount = 0;

    QDateTime date = QDateTime::currentDateTime();
    ui->textEdit_Log->append("============ Date : " + date.toString("yyyy-MM-dd HH:mm:ss"));

    plot = ui->powerPlot;
    plot->xAxis->setLabel("freq. [MHz]");
    plot->yAxis->setLabel("power. [a.u.]");
    plot->xAxis->setRange(900, 2100);
    plot->addGraph();
    plot->graph(0)->setPen(QPen(Qt::blue));
    plot->replot();

    ui->lineEdit_Start->setText("1000");
    ui->lineEdit_Stop->setText("2000");
    ui->lineEdit_Points->setText("101");
    ui->lineEdit_Dwell->setText("200");
    ui->lineEdit_StepSize->setText("10 MHz");
    ui->lineEdit_RunTime->setText("~20.100 sec");

    //============== opne power meter
    powerMeter = new QSCPI("USB0::0x2A8D::0x1601::MY53102568::0::INSTR"); // DMM
    connect(powerMeter, SIGNAL(SendMsg(QString)), this, SLOT(LogMsg(QString)));
    if( powerMeter->status == VI_SUCCESS){
        LogMsg("power meter online.");
        ui->pushButton_ReadPower->setEnabled(true);
        sprintf(powerMeter->cmd, ":configure:voltage:DC\n");
        powerMeter->SendCmd(powerMeter->cmd);
    }else{
        LogMsg("Power meter cannot be found.");
        ui->pushButton_ReadPower->setEnabled(false);
    }

    //============= open generator;
    generatorPortName = "";
    findSeriesPortDevices(); // this locate the generatorPortName;

    generator = new QSerialPort(this);    
    generator->setPortName(generatorPortName);
    generator->setBaudRate(QSerialPort::Baud115200);
    generator->setDataBits(QSerialPort::Data8);
    generator->setParity(QSerialPort::NoParity);
    generator->setStopBits(QSerialPort::OneStop);
    generator->setFlowControl(QSerialPort::NoFlowControl);

    connect(ui->lineEdit, SIGNAL(returnPressed()), this, SLOT(on_pushButton_SendCommand_clicked()));
    //connect(generator, static_cast<void (QSerialPort::*)(QSerialPort::SerialPortError)>(&QSerialPort::error),
    //        this, &MainWindow::handleError);
    connect(generator, &QSerialPort::readyRead, this, &MainWindow::readFromDevice);

    if(generator->open(QIODevice::ReadWrite)){
        LogMsg("The generator is connected in " + generatorPortName + ".");
        ui->statusBar->setToolTip( tr("The generator is connected."));
        controlOnOFF(true);
        write2Device("OUTP:STAT OFF");
    }else{
        //QMessageBox::critical(this, tr("Error"), generator->errorString());
        LogMsg("The generator cannot be found on any COM port.");
        ui->statusBar->setToolTip(tr("Open error"));
        controlOnOFF(false);
    }

}

MainWindow::~MainWindow()
{
    if(generator->isOpen()) generator->close();
    qDebug("generator closed.");

    delete powerMeter;
    delete generator;
    delete plot;
    delete ui;
}

void MainWindow::LogMsg(QString str)
{
    msgCount ++;
    QString dateStr = QDateTime::currentDateTime().toString("HH:mm:ss ");
    QString countStr;
    countStr.sprintf("[%04d]: ", msgCount);
    str.insert(0, countStr).insert(0, dateStr);
    ui->textEdit_Log->append(str);
    int max = ui->textEdit_Log->verticalScrollBar()->maximum();
    ui->textEdit_Log->verticalScrollBar()->setValue(max);
}

void MainWindow::on_pushButton_Sweep_clicked()
{

    controlOnOFF(false);
    ui->pushButton_ReadPower->setEnabled(false);

    ui->pushButton_Sweep->setStyleSheet("background-color: rgb(0,255,0)");
    write2Device("*BUZZER OFF");

    write2Device("OUTP:STAT ON"); // switch on RF
    //Looping
    QString stepstr = ui->lineEdit_StepSize->text();
    stepstr.chop(3);
    double step = stepstr.toDouble();
    double start = ui->lineEdit_Start->text().toDouble();
    //double stop = ui->lineEdit_Stop->text().toDouble();
    int points = ui->lineEdit_Points->text().toInt();
    double waitTime = ui->lineEdit_Dwell->text().toDouble(); // in ms;

    double yMin, xMin;

    x.clear();
    y.clear();
    qDebug() << points << ", " << step;
    for( int i = 1 ; i <= points; i ++){
        double freq = start + (i-1) * step;
        qDebug() << i << "," <<  freq;
        QString input;
        input.sprintf("FREQ:CW %fMHz", freq);
        write2Device(input);

        x.push_back(freq);

        //wait for waitTime
        QEventLoop eventLoop;
        QTimer::singleShot(waitTime, &eventLoop, SLOT(quit()));
        eventLoop.exec();

        //get powerMeter reading;
        sprintf(powerMeter->cmd, ":READ?\n");
        double reading = powerMeter->Ask(powerMeter->cmd).toDouble();
        LogMsg("reading : " + QString::number(reading));
        y.push_back(reading);
        if( i == 1) {
            yMin = reading;
            xMin = freq;
        }else{
            if( yMin > reading ){
                yMin = reading;
                xMin = freq;
            }
        }
        // plotgraph
        plot->graph(0)->clearData();
        plot->graph(0)->setData(x,y);
        plot->yAxis->rescale();
        plot->replot();

    }

    write2Device("OUTP:STAT OFF"); // switch off RF
    write2Device("*BUZZER ON");
    ui->pushButton_Sweep->setStyleSheet("");

    controlOnOFF(true);
    ui->pushButton_ReadPower->setEnabled(true);

    QString msg;
    msg.sprintf("Min(x,y) = (%f, %f)", xMin, yMin);
    LogMsg(msg);

}

void MainWindow::findSeriesPortDevices()
{
    //static const char blankString[] = QT_TRANSLATE_NOOP("SettingsDialog", "N/A");
    LogMsg("--------------");
    LogMsg("found COM Ports:");
    const auto infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : infos) {
        //LogMsg("PortName     ="+info.portName()                                                                         );
        //LogMsg("description  ="+(!info.description().isEmpty() ?  info.description() : blankString)                                    );
        //LogMsg("manufacturer ="+(!info.manufacturer().isEmpty() ? info.manufacturer() : blankString)                                  );
        //LogMsg("serialNumber ="+(!info.serialNumber().isEmpty() ? info.serialNumber() : blankString)                                  );
        //LogMsg("Location     ="+info.systemLocation()                                                                   );
        //LogMsg("Vendor       ="+(info.vendorIdentifier() ? QString::number(info.vendorIdentifier(), 16) : blankString)  );
        //LogMsg("Identifier   ="+(info.productIdentifier() ? QString::number(info.productIdentifier(), 16) : blankString));
        //LogMsg("=======================");

        LogMsg(info.portName() + ", " + info.serialNumber() + ", " + info.manufacturer());

        if(info.serialNumber() == "DQ000VJLA" && info.manufacturer() == "FTDI" ){
            generatorPortName = info.portName();
        }
    }
    LogMsg ("--------------");
}

void MainWindow::write2Device(const QString &msg)
{
    const QString temp = msg + "\n";
    generator->write(temp.toStdString().c_str());
    LogMsg("write : " + msg);
}

void MainWindow::readFromDevice()
{
    QByteArray read = generator->readAll();
    LogMsg("ANS = " + QString(read));
    generatorLastRepond = QString(read);
}

void MainWindow::controlOnOFF(bool IO)
{
    ui->lineEdit->setEnabled(IO);
    ui->pushButton_SendCommand->setEnabled(IO);
    ui->doubleSpinBox->setEnabled(IO);
    ui->doubleSpinBox_Power->setEnabled(IO);
    ui->lineEdit_Dwell->setEnabled(IO);
    ui->lineEdit_Points->setEnabled(IO);
    ui->lineEdit_Start->setEnabled(IO);
    ui->lineEdit_Stop->setEnabled(IO);
    ui->pushButton_Sweep->setEnabled(IO);
}

void MainWindow::on_pushButton_SendCommand_clicked()
{
    write2Device(ui->lineEdit->text());
}

void MainWindow::on_lineEdit_Points_textChanged(const QString &arg1)
{
    double start = ui->lineEdit_Start->text().toDouble();
    double stop = ui->lineEdit_Stop->text().toDouble();
    double step = (stop-start) / (arg1.toInt()-1);
    double time = ui->lineEdit_Dwell->text().toDouble();
    double runTime = arg1.toInt() * time / 1000.;
    ui->lineEdit_StepSize->setText(QString::number(step) + " MHz");
    ui->lineEdit_RunTime->setText(QString::number(runTime) + " sec");
}

void MainWindow::on_lineEdit_Dwell_textChanged(const QString &arg1)
{
    int points = ui->lineEdit_Points->text().toInt();
    double runTime = points * arg1.toDouble() / 1000.;
    ui->lineEdit_RunTime->setText("~" + QString::number(runTime) + " sec");
    if( arg1.toDouble() < 100){
        LogMsg("The dwell time may be too small for the power meter to respond.");
        LogMsg("Please consider to increase the dwell time.");
    }
}

void MainWindow::on_lineEdit_Start_textChanged(const QString &arg1)
{
    double stop = ui->lineEdit_Stop->text().toDouble();
    int points = ui->lineEdit_Points->text().toInt();
    double range = (stop - arg1.toDouble());
    double step = range/(points-1);
    ui->lineEdit_StepSize->setText(QString::number(step) + " MHz");

    plot->xAxis->setRange(arg1.toDouble() - range*0.1, stop +  range*0.1);
    plot->replot();
}

void MainWindow::on_lineEdit_Stop_textChanged(const QString &arg1)
{
    double start = ui->lineEdit_Start->text().toDouble();
    int points = ui->lineEdit_Points->text().toInt();
    double range = (arg1.toDouble() - start);
    double step = range/(points-1);
    ui->lineEdit_StepSize->setText(QString::number(step) + " MHz");

    plot->xAxis->setRange(start - range*0.1, arg1.toDouble()+  range*0.1);
    plot->replot();
}

void MainWindow::on_doubleSpinBox_Power_valueChanged(double arg1)
{
    write2Device("PWR " + QString::number(arg1));
}

void MainWindow::on_doubleSpinBox_valueChanged(double arg1)
{
    write2Device("ATT " + QString::number(arg1));
}

void MainWindow::on_actionSave_Data_triggered()
{
    int size = x.size();

    if( x.size() != y.size()) {
        LogMsg("data corrupt. Please measure again.");
        return;
    }

    if( x.size() == 0){
        LogMsg("no data to save.");
        return;
    }

    QDateTime date = QDateTime::currentDateTime();
    QString fileName = date.toString("yyyyMMdd_HHmmss") + "DNP100GHz.dat";
    QString filePath = DESKTOP_PATH + "/" + fileName;
    QFile outfile(filePath);

    outfile.open(QIODevice::WriteOnly);

    QTextStream stream(&outfile);
    QString lineout;

    lineout.sprintf("###%s", date.toString("yyyy-MM-dd HH:mm:ss\n").toStdString().c_str());
    stream << lineout;
    lineout.sprintf("###number of data %d\n", size);
    stream << lineout;
    lineout.sprintf("%10s\t%10s\n", "freq.[Mhz]", "Power");
    stream << lineout;

    for( int i = 0; i < size; i++){
        lineout.sprintf("%10f\t%10f\n", x[i], y[i]);
        stream << lineout;
    }

    stream << "========= end of file =======";
    outfile.close();

    LogMsg("saved data to DESKTOP: " + fileName);
}

void MainWindow::on_pushButton_ReadPower_clicked()
{
    sprintf(powerMeter->cmd, ":READ?\n");
    powerMeter->Ask(powerMeter->cmd).toDouble();
    //LogMsg("reading : " + QString::number(reading));
}

void MainWindow::on_actionSave_plot_triggered()
{
    if( x.size() != y.size()) {
        LogMsg("data corrupt. Please measure again.");
        return;
    }

    if( x.size() == 0){
        LogMsg("no data to save.");
        return;
    }

    QFileDialog fileDialog(this);
    QStringList nameFilterList = {"pdf (*.pdf)", "PNG (*.png)"};
    fileDialog.setNameFilters(nameFilterList);
    fileDialog.setDirectory(DESKTOP_PATH);
    fileDialog.setReadOnly(0);
    QString fileName;
    if( fileDialog.exec()){
        fileName = fileDialog.selectedFiles()[0];
    }

    bool ok = false;
    int ph = plot->geometry().height();
    int pw = plot->geometry().width();
    if( fileDialog.selectedNameFilter() == nameFilterList[0]){
        if( fileName.right(4) != ".pdf" ) fileName.append(".pdf");
        ok = plot->savePdf(fileName, pw, ph );
    }else{
        if( fileName.right(4) != ".png" ) fileName.append(".png");
        ok = plot->savePng(fileName, pw, ph );
    }

    if( ok ){
        LogMsg("Saved Time-Plot as " + fileName);
    }else{
        LogMsg("Save Failed.");
    }
}


void MainWindow::on_pushButton_RFOnOff_clicked()
{

}
