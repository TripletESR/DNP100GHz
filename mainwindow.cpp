#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    plot = ui->powerPlot;
    plot->xAxis->setLabel("freq. [MHz]");
    plot->yAxis->setLabel("power. [a.u.]");
    plot->xAxis->setRange(900, 2100);
    plot->addGraph();
    plot->graph(0)->setPen(QPen(Qt::blue));
    plot->replot();

    RFOnOff = false;

    ui->lineEdit_Start->setText("1000");
    ui->lineEdit_Stop->setText("2000");
    ui->lineEdit_Points->setText("101");
    ui->lineEdit_Dwell->setText("5");
    ui->lineEdit_StepSize->setText("10 MHz");
    ui->lineEdit_RunTime->setText("~0.505 sec");

    connect(ui->lineEdit, SIGNAL(returnPressed()), this, SLOT(on_pushButton_SendCommand_clicked()));

    powerMeter = NULL;
    generatorPortName = "";
    findSeriesPortDevices(); // this locate the generatorPortName;

    //============= open generator;
    generator = new QSerialPort(this);    
    generator->setPortName(generatorPortName);
    generator->setBaudRate(QSerialPort::Baud115200);
    generator->setDataBits(QSerialPort::Data8);
    generator->setParity(QSerialPort::NoParity);
    generator->setStopBits(QSerialPort::OneStop);
    generator->setFlowControl(QSerialPort::NoFlowControl);

    //connect(generator, static_cast<void (QSerialPort::*)(QSerialPort::SerialPortError)>(&QSerialPort::error),
    //        this, &MainWindow::handleError);
    connect(generator, &QSerialPort::readyRead, this, &MainWindow::readFromDevice);

    if(generator->open(QIODevice::ReadWrite)){
        LogMsg(" The generator is connected in " + generatorPortName + ".");
        ui->statusBar->setToolTip( tr("The generator is connected."));
        controlOnOFF(true);
    }else{
        QMessageBox::critical(this, tr("Error"), generator->errorString());
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
    ui->textEdit_Log->append(str);
    int max = ui->textEdit_Log->verticalScrollBar()->maximum();
    ui->textEdit_Log->verticalScrollBar()->setValue(max);
}

void MainWindow::on_pushButton_RFonoff_clicked()
{
    if(RFOnOff){
        RFOnOff = false;
    }else{
        RFOnOff = true;
    }

    //qDebug() << ui->pushButton_RFonoff->styleSheet();

    if(RFOnOff){
        ui->pushButton_RFonoff->setStyleSheet("background-color: rgb(0,255,0)");
    }else{
        ui->pushButton_RFonoff->setStyleSheet("");
    }

    controlOnOFF(!RFOnOff);

    if(RFOnOff){
        write2Device("*BUZZER OFF");

        //write2Device("OUTP:STAT ON"); // switch on RF
        //Looping
        QString stepstr = ui->lineEdit_StepSize->text();
        stepstr.chop(3);
        double step = stepstr.toDouble();
        double start = ui->lineEdit_Start->text().toDouble();
        //double stop = ui->lineEdit_Stop->text().toDouble();
        int points = ui->lineEdit_Points->text().toInt();
        double waitTime = ui->lineEdit_Dwell->text().toDouble(); // in ms;

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
            y.push_back(sin(0.1*i));

            // plotgraph
            plot->graph(0)->clearData();
            plot->graph(0)->setData(x,y);
            plot->yAxis->rescale();
            plot->replot();

        }

        //write2Device("OUTP:STAT OFF"); // switch off RF
        write2Device("*BUZZER ON");
    }

}

void MainWindow::findSeriesPortDevices()
{
    //static const char blankString[] = QT_TRANSLATE_NOOP("SettingsDialog", "N/A");

    LogMsg("found Ports:");
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

        LogMsg(info.portName());

        if(info.serialNumber() == "DQ000VJLA" && info.manufacturer() == "FTDI" ){
            generatorPortName = info.portName();
        }
    }
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
    ui->lineEdit_RunTime->setText(QString::number(runTime) + " sec");
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

}
