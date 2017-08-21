#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    plot = ui->powerPlot;
    RFOnOff = false;

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
    }else{
        QMessageBox::critical(this, tr("Error"), generator->errorString());

        ui->statusBar->setToolTip(tr("Open error"));
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
    write2Device(ui->lineEdit->text());

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

QString MainWindow::readFromDevice()
{
    QByteArray read = generator->readAll();

    LogMsg("ANS = " + QString(read));
    return QString(read);
}
