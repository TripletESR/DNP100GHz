#include "mainwindow.h"
#include "ui_mainwindow.h"

const QString DESKTOP_PATH = QStandardPaths::locate(QStandardPaths::DesktopLocation, QString(), QStandardPaths::LocateDirectory);
const QString DATA_PATH = "Z:/triplet/";

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    rfOnOff = false;
    sweepOnOff = 0;
    msgCount = 0;
    stopdBm2mW = false;
    stopmW2dBm = false;

    z = NULL;

    helpDialog = new QDialog(this);
    HelpLabel = new QLabel();
    helpDialog->setWindowTitle("Help");
    QImage image(":fig.PNG");
    HelpLabel->setPixmap(QPixmap::fromImage(image));

    QVBoxLayout * helpDialogLayout = new QVBoxLayout(helpDialog);
    helpDialogLayout->addWidget(HelpLabel);

    QDateTime date = QDateTime::currentDateTime();
    ui->textEdit_Log->append("============ Date : " + date.toString("yyyy-MM-dd HH:mm:ss"));

    plot = ui->powerPlot;
    auxPlot = ui->comparePlot;
    colorMap = NULL;
    colorScale = NULL;
    ui->actionSave_2_D_plot->setEnabled(false);
    ui->checkBox_Normalize->setEnabled(false);

    QMessageBox msgBox;
    msgBox.setWindowTitle("Program Mode.");
    QString boxMsg;

    //##########################################################
    //============== opne power meter
    powerMeter = new QSCPI((ViRsrc) "GPIB0::4::INSTR"); // the DPM

    if( powerMeter->status == VI_SUCCESS){
        connect(powerMeter, SIGNAL(SendMsg(QString)), this, SLOT(LogMsg(QString)));
        LogMsg("DPM online.");
        hasPowerMeter = true;

        ui->pushButton_ReadPower->setEnabled(true);
        ui->spinBox_AveragePoints->setEnabled(true);
        ui->pushButton_GetNoPoint->setEnabled(true);
        //get number of point
        on_pushButton_GetNoPoint_clicked();

        boxMsg += "* Digital Power Meter (DPM) is online.\n";

    }else{
        LogMsg("DPM cannot be found or open.", Qt::red);
        powerMeter->ErrorMassage();
        LogMsg("VISA Error : " + powerMeter->scpi_Msg, Qt::red);
        hasPowerMeter = false;

        ui->spinBox_AveragePoints->setEnabled(false);
        ui->pushButton_GetNoPoint->setEnabled(false);
    }

    //=============== open DMM
    DMM = new QSCPI((ViRsrc) "USB0::0x2A8D::0x1601::MY53102568::0::INSTR"); // DMM
    if( DMM->status == VI_SUCCESS){
        connect(DMM, SIGNAL(SendMsg(QString)), this, SLOT(LogMsg(QString)));
        LogMsg("DMM is online.");
        hasDMM = true;

        ui->pushButton_ReadPower->setEnabled(true);

        boxMsg += "* Digital Mulit-Meter (DMM) is online.\n";

    }else{
        LogMsg("DMM cannot be found or open.", Qt::red);
        DMM->ErrorMassage();
        LogMsg("VISA Error : " + DMM->scpi_Msg, Qt::red);
        hasDMM = false;

    }

    if( !hasPowerMeter && !hasDMM){
        ui->pushButton_ReadPower->setEnabled(false);

        boxMsg += "* No meter is online.\n";

    }

    //##########################################################
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
    connect(generator, &QSerialPort::readyRead, this, &MainWindow::readFromGenerator);

    if(generator->open(QIODevice::ReadWrite)){
        if( generatorType == SG6000LDQ){
            LogMsg("The SG6000LDQ is connected in " + generatorPortName + ".");
        }
        if( generatorType == HMCT2220){
            LogMsg("The HMC-T2220 is connected in " + generatorPortName + ".");
        }
        ui->statusBar->setToolTip( tr("The generator is connected."));
        controlOnOFF(true);
        ui->pushButton_Sweep->setEnabled(true);
        write2Generator("OUTP:STAT OFF");

        if( generatorType == SG6000LDQ){
            ui->lineEdit_PowerStart->setEnabled(false);
            ui->lineEdit_PowerEnd->setEnabled(false);
            ui->spinBox_PowerStep->setEnabled(false);

            ui->label_Power->setText("Power [%]");
            ui->doubleSpinBox_Power->setMinimum(0);
            ui->doubleSpinBox_Power->setMaximum(100);
            ui->doubleSpinBox_Power->setSingleStep(1);
            ui->doubleSpinBox_Power->setValue(100);
        }

        if( generatorType == HMCT2220){
            ui->lineEdit_PowerStart->setEnabled(true);
            ui->lineEdit_PowerEnd->setEnabled(true);
            ui->spinBox_PowerStep->setEnabled(true);

            ui->doubleSpinBox->setEnabled(false);
            // change the name of the power line_edit
            ui->label_Power->setText("Power [dBm]"); // 1 dBm = 10 * Log(P/1mW), P = 1mW * 10^(dBm/10)
            ui->doubleSpinBox_Power->setMinimum(-50);
            ui->doubleSpinBox_Power->setMaximum(20);
            ui->doubleSpinBox_Power->setSingleStep(1);
            ui->doubleSpinBox_Power->setValue(-50);
        }

        if( generatorCount == 1){
            if( generatorType == SG6000LDQ){
                boxMsg += "* SG6000LDQ generator is online.\n";
            }
            if( generatorType == HMCT2220){
                boxMsg += "* HMC-T2220 is online.\n";
            }
        }else if(generatorCount > 1){
            boxMsg += "* SG6000LDQ and HMC-T2220 are online.\n";
            boxMsg += "       --> HMC-T2220 is used.\n";
        }else{
            boxMsg += "* No generator is online.\n";
        }

    }else{
        //QMessageBox::critical(this, tr("Error"), generator->errorString());
        LogMsg("Generator cannot be found on any COM port.", Qt::red);
        ui->statusBar->setToolTip(tr("Open error"));
        controlOnOFF(false);
        ui->pushButton_RFOnOff->setEnabled(false);
        ui->pushButton_Sweep->setEnabled(false);
        ui->doubleSpinBox->setEnabled(false);

        ui->lineEdit_PowerStart->setEnabled(false);
        ui->lineEdit_PowerEnd->setEnabled(false);
        ui->spinBox_PowerStep->setEnabled(false);

        boxMsg += "* No generator is online.\n";
    }

    //================== connect switch matrix
    switchConnected = false;
    switchMatrix = new QAxObject("MCL_RF_Switch_Controller.USB_RF_Switch");

    if( switchMatrix->dynamicCall("Connect()").toInt()){

        QList<QVariant> list;
        list << "dummy";
        switchMatrix->dynamicCall("Read_ModelName(QString&)", list);
        QString modelName = list[0].toString();
        switchMatrix->dynamicCall("Read_SN(QString&)", list);
        QString serialNumber = list[0].toString();
        LogMsg("Switch connected: " + modelName + ", S/N: " + serialNumber);

        switchMatrix->dynamicCall("GetSwitchesStatus(int&)", list);
        int port = list[0].toInt();

        LogMsg("Switch A port = " + QString::number((port & 1)+1));
        LogMsg("Switch B port = " + QString::number((port>>1 )+1));

        ui->horizontalSlider_A->setValue(port & 1);
        ui->horizontalSlider_B->setValue(port & 2);

        switchConnected = true;

        boxMsg += "* Switch Matrix is online.\n";
    }else{
        LogMsg("Connot find switch matrix.", Qt::red);
        ui->horizontalSlider_A->setEnabled(false);
        ui->horizontalSlider_B->setEnabled(false);
        ui->checkBox_Sync->setEnabled(false);

        boxMsg += "* Switch Matrix is off-line.\n";
    }

    qDebug() << "has POwer Meter ?" << hasPowerMeter;
    qDebug() << "has DMM ?" << hasDMM;
    qDebug() << "generator Type ?" << generatorType;
    qDebug() << "samll generator ? " << (generatorType == SG6000LDQ);
    qDebug() << "HMC-T2220 generator ? " << (generatorType == HMCT2220);

    //###################### Program mode depends on connected device

    boxMsg += "\n";
    LogMsg(" ######################### ");
    QString title_1 = "DNP 100GHz - ";
    QString title_2;
    if( generatorType == SG6000LDQ && (hasPowerMeter ^ hasDMM)) {
        programMode = 1;
        if(hasPowerMeter){
            title_2 = "Program mode = 1, simple measurement with SG6000LDQ + DPM";
        }
        if(hasDMM){
            title_2 = "Program mode = 1, simple measurement with SG6000LDQ + DMM";
            ui->spinBox_Dwell->setMinimum(0);
        }
    }else if( hasPowerMeter && hasDMM) {
        programMode = 2;
        title_2 = "Program mode = 2, Calibration.";
        if( generatorCount > 1){
            LogMsg("Two generators are online, using HMC-T2220.", Qt::blue);
        }
        LogMsg("If you DON'T want to be calibration, please disconnect the DPM or DMM.", Qt::blue);
    }else if( generatorType == HMCT2220 && (hasPowerMeter ^ hasDMM)) {
        programMode = 3;
        if(hasPowerMeter){
            title_2 = "Program mode = 3, Measurement with HMC-T2220 + DPM";
        }
        if(hasDMM){
            title_2 = "Program mode = 3, Measurement with HMC-T2220 + DMM";
            ui->spinBox_Dwell->setMinimum(0);
        }
    }else {
        programMode = 4;
        title_2 = "Program mode = 4. Only generator is online or no device.";
        ui->spinBox_Dwell->setMinimum(0);
    }

    LogMsg(title_2, Qt::blue);
    this->setWindowTitle(title_1 + title_2 );

    boxMsg += title_2;
    msgBox.setText(boxMsg);
    msgBox.exec();

    //test
    //programMode = 3;
    //hasDMM = true;
    //ui->pushButton_Sweep->setEnabled(true);

    qDebug() << "program Mode ?" << programMode;

    //########################## Set up the plot according to the program mode

    if( programMode == 1){
        plot->xAxis->setLabel("freq. [GHz]");

        plot->addGraph();
        plot->graph(0)->setPen(QPen(Qt::blue)); // for y, DPM

        if( hasPowerMeter ){
            plot->graph(0)->setName("DPM");
            plot->yAxis->setLabel("DPM power [mW]");
        }
        if( hasDMM ) {
            plot->graph(0)->setName("DMM");
            plot->yAxis->setLabel("DMM power [mV]");
        }
        plot->legend->setVisible(true);
        plot->replot();

    }

    if( programMode == 2){
        plot->xAxis->setLabel("freq. [GHz]");
        plot->yAxis->setLabel("DPM power [mW]");
        plot->addGraph();
        plot->graph(0)->setPen(QPen(Qt::blue)); // for y, DPM
        plot->graph(0)->setName("DPM");

        plot->addGraph(plot->xAxis, plot->yAxis2);
        plot->graph(1)->setPen(QPen(Qt::red)); // for y2, DMM
        plot->graph(1)->setName("DMM");
        plot->yAxis2->setVisible(true);
        plot->yAxis2->setLabel("DMM power [mV]");
        plot->legend->setVisible(true);
        plot->replot();

        auxPlot->xAxis->setLabel("DPM [mW]");
        auxPlot->yAxis->setLabel("DMM [mV]");
        auxPlot->addGraph();
        auxPlot->graph(0)->setPen(QPen(Qt::blue)); // for y, DPM
        auxPlot->graph(0)->setLineStyle(QCPGraph::lsNone);
        auxPlot->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCross, 5));

        auxPlot->replot();

        ui->lineEdit_PowerStart->setEnabled(false);
        ui->lineEdit_PowerEnd->setEnabled(false);
        ui->spinBox_PowerStep->setEnabled(false);

        ui->actionSave_2_D_plot->setEnabled(true);
    }

    if( programMode == 3 ){
        plot->xAxis->setLabel("freq. [GHz]");
        plot->addGraph();
        plot->legend->setVisible(true);
        plot->replot();
        auxPlot->xAxis->setLabel("freq. [GHz]");
        auxPlot->yAxis->setLabel("Input power [dBm]");
        colorMap = new QCPColorMap(auxPlot->xAxis, auxPlot->yAxis);

        colorMap->data()->setRange(QCPRange(3.9*x4*x6, 4.0*x4*x6), QCPRange(-50,10));// set the x, y axis range
        colorMap->rescaleAxes();

        colorScale = new QCPColorScale(auxPlot);
        auxPlot->plotLayout()->addElement(0, 1, colorScale); // add it to the right of the main axis rect
        colorScale->setType(QCPAxis::atRight); // scale shall be vertical bar with tick/axis labels right (actually atRight is already the default)
        colorMap->setColorScale(colorScale); // associate the color map with the color scale
        if( hasPowerMeter){
            colorScale->axis()->setLabel("DPM power [mW]");
        }
        if( hasDMM){
            colorScale->axis()->setLabel("DMM power [mV]");
        }

        auxPlot->replot();

        ui->actionSave_2_D_plot->setEnabled(true);
        ui->checkBox_Normalize->setEnabled(true);
        ui->checkBox_Normalize->setChecked(false);
        on_checkBox_Normalize_clicked(false);
    }

    ui->verticalSlider_power->setEnabled(false);
    ui->verticalSlider_power->setMaximum(1);
    ui->verticalSlider_power->setMinimum(1);

    ui->lineEdit_Start->setText("3.9");
    ui->lineEdit_Stop->setText("4.0");
    ui->spinBox_Points->setValue(101);
    ui->spinBox_Dwell->setValue(400);
    if(!hasPowerMeter && hasDMM ) ui->spinBox_Dwell->setValue(0);
    if(programMode == 4 ) ui->spinBox_Dwell->setValue(0);
    if(generatorType == SG6000LDQ) ui->spinBox_Dwell->setValue(20);
    ui->lineEdit_StepSize->setText("1 MHz");
    ui->lineEdit_RunTime->setText("~20.100 sec");
    ui->lineEdit_Multiplier->setText(QString::number(x4*x6));
    ui->lineEdit_EffStart->setText(QString::number(3.900*x4*x6));
    ui->lineEdit_EffStop->setText(QString::number(4.000*x4*x6));
    ui->lineEdit_EffStart_d6->setText(QString::number(3.900*x4));
    ui->lineEdit_EffStop_d6->setText(QString::number(4.000*x4));
    ui->lineEdit_Freq->setText("4.000");
    ui->lineEdit_EffFreq->setText(QString::number(4.000*x4*x6));

    ui->label_EffFreq_d6->setText("Eff. Freq. / "+ QString::number(x6) + " [GHz]");

    ui->comboBox_yAxis->addItem("Linear y");
    ui->comboBox_yAxis->addItem("Log y");
    ui->comboBox_yAxis->addItem("dB y");

    ui->lineEdit_PowerStart->setText("-50");
    ui->lineEdit_PowerEnd->setText("10");
    ui->spinBox_PowerStep->setValue(101);

    ui->comboBox_yAxis->setEnabled(false);
    ui->spinBox_Average->setEnabled(false);

    if(programMode == 3){
        colorMap->data()->setKeyRange(QCPRange( 3.900*x4*x6, 4.000*x4*x6));
        colorMap->data()->setSize(101, 101); // this is just the grid size.
        colorMap->rescaleAxes();
        auxPlot->replot();
    }

}

MainWindow::~MainWindow()
{
    if(generator->isOpen()) generator->close();

    if(switchConnected){
        switchMatrix->dynamicCall("Disconnect()");
    }

    if(z != NULL) delete z;
    delete switchMatrix;
    delete powerMeter;
    delete DMM;
    delete generator;
    delete plot;
    if(colorScale != NULL) delete colorScale;
    if(colorMap != NULL) delete colorMap;
    delete auxPlot;
    delete ui;
}

void MainWindow::LogMsg(QString str, QColor color)
{
    msgCount ++;
    QString dateStr = QDateTime::currentDateTime().toString("HH:mm:ss ");
    QString countStr;
    countStr.sprintf("[%04d]: ", msgCount);
    str.insert(0, countStr).insert(0, dateStr);

    ui->textEdit_Log->setTextColor(color);
    ui->textEdit_Log->append(str);
    ui->textEdit_Log->setTextColor(QColor(0,0,0));

    int max = ui->textEdit_Log->verticalScrollBar()->maximum();
    ui->textEdit_Log->verticalScrollBar()->setValue(max);
}

void MainWindow::on_pushButton_Sweep_clicked()
{
    sweepOnOff = !sweepOnOff;

    if( sweepOnOff ){
        LogMsg("======= Sweeping Start! ", Qt::blue);
        controlOnOFF(false);
        ui->pushButton_ReadPower->setEnabled(false);
        ui->pushButton_RFOnOff->setEnabled(false);
        ui->comboBox_yAxis->setEnabled(false);
        ui->spinBox_Average->setEnabled(false);
        ui->spinBox_AveragePoints->setEnabled(false);
        ui->pushButton_GetNoPoint->setEnabled(false);
        ui->verticalSlider_power->setEnabled(false);
        ui->checkBox_Normalize->setEnabled(false);

        ui->pushButton_Sweep->setStyleSheet("background-color: rgb(0,255,0)");
        if(generatorType == SG6000LDQ) write2Generator("*BUZZER OFF");

        write2Generator("OUTP:STAT ON"); // switch on RF

        //Looping===================
        QString stepstr = ui->lineEdit_StepSize->text();
        stepstr.chop(3);
        double step = stepstr.toDouble() / 1000; //in GHz
        double start = ui->lineEdit_Start->text().toDouble();
        //double stop = ui->lineEdit_Stop->text().toDouble();
        int points = ui->spinBox_Points->value();
        double waitTime = ui->spinBox_Dwell->value(); // in ms;

        double yMin = 0., xMin = 0.;

        int multi = ui->lineEdit_Multiplier->text().toInt();

        LogMsg("Clearing data.");
        x.clear();
        y.clear();
        y2.clear();

        if(programMode != 3){
            if( programMode == 4 ){ // only sweep
                if(generatorType == HMCT2220){
                    double powerStart = ui->lineEdit_PowerStart->text().toDouble();
                    double powerEnd = ui->lineEdit_PowerEnd->text().toDouble();
                    int powerStep = ui->spinBox_PowerStep->value();
                    double powerStepSize = (powerEnd-powerStart)/(powerStep-1);


                    for( int j = 1; j <= powerStep; j++){
                        if(!sweepOnOff) goto endloop;
                        double power = powerStart + (j-1) * powerStepSize;
                        //set HMC-T2220 power
                        write2Generator("Power " + QString::number(power));

                        for( int i = 1 ; i <= points; i ++){
                            if(!sweepOnOff) goto endloop;
                            double freq = start + (i-1) * step;
                            qDebug() << "(" << i << "," << j << ") = (" <<  freq << " GHz," << power <<" dBm)";

                            // set generator freqeuncey
                            QString input;
                            input.sprintf("FREQ %fGHz", freq*multi/x6);
                            write2Generator(input);

                            //wait for waittime, for the powereter to measure the freq.
                            QEventLoop eventLoop;
                            QTimer::singleShot(waitTime, &eventLoop, SLOT(quit()));
                            eventLoop.exec();


                        } // end of for-loop for freqeuncy
                    } // end of for-loop for power

                }else if(generatorType == SG6000LDQ){
                    for( int i = 1 ; i <= points; i ++){
                        if(!sweepOnOff) goto endloop;
                        double freq = start + (i-1) * step;
                        qDebug() <<  i << "," <<  freq << " GHz" ;

                        // set generator freqeuncey
                        QString input;
                        input.sprintf("FREQ:CW %fMHz", freq*1000.);
                        write2Generator(input);

                        //wait for waittime, for the powereter to measure the freq.
                        QEventLoop eventLoop;
                        QTimer::singleShot(waitTime, &eventLoop, SLOT(quit()));
                        eventLoop.exec();


                    } // end of for-loop for freqeuncy
                }else{
                    return;
                }

            }else{ // programMode == 1, 2
                for( int i = 1 ; i <= points; i ++){
                    if(!sweepOnOff) goto endloop;

                    double freq = start + (i-1) * step;
                    qDebug() << i << "," <<  freq << "GHz";

                    // set generator freqeuncey
                    if( generatorType == SG6000LDQ){
                        QString input;
                        input.sprintf("FREQ:CW %fMHz", freq*1000.);
                        write2Generator(input);
                    }
                    if( generatorType == HMCT2220){
                        QString input;
                        input.sprintf("FREQ %fMHz", freq*1000.*multi/x6);
                        write2Generator(input);
                    }
                    x.push_back(freq * multi);

                    double readingPM, readingDMM;
                    //get powerMeter reading; change the DPM freq., then read
                    if( hasPowerMeter ){
                        QString freqCmd = "sens:freq ";
                        QString freqStr;
                        freqStr.sprintf("%6.2f",freq*x4*x6); // in GHz, also with 4 and 6 mulipiler.
                        freqStr.remove(" ");
                        freqCmd = freqCmd + freqStr;
                        //qDebug() << "-----------" << freqCmd;
                        sprintf(powerMeter->cmd, "%s\n", freqCmd.toStdString().c_str());
                        powerMeter->SendCmd(powerMeter->cmd);

                        //wait for waittime, for the powereter to measure the freq.
                        QEventLoop eventLoop;
                        QTimer::singleShot(waitTime, &eventLoop, SLOT(quit()));
                        eventLoop.exec();

                        sprintf(powerMeter->cmd, "READ?\n");
                        QString readingStr = powerMeter->Ask(powerMeter->cmd);
                        QString unit = readingStr.right(2);
                        //qDebug() << "................." << readingStr << ", unit : " << unit;
                        readingStr.chop(2);
                        readingPM = readingStr.toDouble();
                        //change the unit to mW for other unit.
                        if( unit == "UW" ) readingPM = readingPM / 1000.;

                        //Warning when power > 20 mW;
                        if( readingPM >= 20. ) {
                            LogMsg("##################################################", Qt::red);
                            LogMsg("####### DANGER! power >= 20 mW !!!  ##############", Qt::red);
                            LogMsg("##################################################", Qt::red);
                        }

                    }

                    if( hasDMM ){
                        sprintf(DMM->cmd, ":READ?\n");
                        readingDMM = DMM->Ask(DMM->cmd).toDouble() * 1000; // to mV

                        //wait for waittime, for the powereter to measure the freq.
                        //QEventLoop eventLoop;
                        //QTimer::singleShot(waitTime, &eventLoop, SLOT(quit()));
                        //eventLoop.exec();
                    }

                    // fill plot
                    if( hasPowerMeter && !hasDMM){
                        y.push_back(readingPM);
                        if( i == 1) {
                            yMin = readingPM;
                            xMin = freq;
                        }

                        if( yMin > readingPM ){
                            yMin = readingPM;
                            xMin = freq;
                        }

                        // plotgraph
                        plot->graph(0)->clearData();
                        plot->graph(0)->setData(x,y);
                        plot->yAxis->rescale();
                        plot->replot();
                    }

                    if( !hasPowerMeter && hasDMM){
                        y.push_back(readingDMM);
                        if( i == 1) {
                            yMin = readingDMM;
                            xMin = freq;
                        }

                        if( yMin > readingDMM ){
                            yMin = readingDMM;
                            xMin = freq;
                        }

                        // plotgraph
                        plot->graph(0)->clearData();
                        plot->graph(0)->setData(x,y);
                        plot->yAxis->rescale();
                        plot->replot();
                    }

                    if(hasPowerMeter && hasDMM){
                        y.push_back(readingPM);
                        y2.push_back(readingDMM);

                        if( i == 1) {
                            yMin = readingPM;
                            xMin = freq;
                        }

                        if( yMin > readingPM ){
                            yMin = readingPM;
                            xMin = freq;
                        }

                        // plotgraph
                        plot->graph(0)->clearData();
                        plot->graph(0)->setData(x,y);
                        plot->yAxis->rescale();

                        plot->graph(1)->clearData();
                        plot->graph(1)->setData(x,y2);
                        plot->yAxis2->rescale();
                        plot->replot();

                        auxPlot->graph(0)->clearData();
                        auxPlot->graph(0)->setData(y,y2);
                        auxPlot->xAxis->rescale();
                        auxPlot->yAxis->rescale();
                        auxPlot->replot();
                    }

                } // end of for-loop for freqeuncy
            }
        }else{ // for programMode == 3

            colorMap->data()->clear();
            y2.clear(); // in this case, y2 save the power;

            double powerStart = ui->lineEdit_PowerStart->text().toDouble();
            double powerEnd = ui->lineEdit_PowerEnd->text().toDouble();
            int powerStep = ui->spinBox_PowerStep->value();
            double powerStepSize = (powerEnd-powerStart)/(powerStep-1);

            if(z != NULL) delete z;
            z = new QVector<double> [powerStep];

            colorMap->data()->setSize(points, powerStep); // this is just the grid size.

            for( int j = 1; j <= powerStep; j++){
                if(!sweepOnOff) goto endloop;
                double power = powerStart + (j-1) * powerStepSize;
                y2.push_back(power);

                //set HMC-T2220 power
                write2Generator("Power " + QString::number(power));

                double normFactor = 1;
                if( ui->checkBox_Normalize->isChecked()) {
                    normFactor = dBm2mW(power);  // in mW
                }

                x.clear();
                y.clear();
                for( int i = 1 ; i <= points; i ++){
                    if(!sweepOnOff) goto endloop;
                    double freq = start + (i-1) * step;
                    qDebug() << "(" << i << "," << j << ") = (" <<  freq << " GHz," << power <<" dBm, " << normFactor << " mW)";

                    // set generator freqeuncey
                    QString input;
                    input.sprintf("FREQ %fMHz", freq * 1000 * multi / x6);
                    write2Generator(input);

                    x.push_back(freq * multi);

                    double readingPM, readingDMM;
                    //get powerMeter reading; change the DPM freq., then read
                    if( hasPowerMeter ){
                        if(!sweepOnOff) goto endloop;
                        if( ui->checkBox_Normalize->isChecked()){
                            plot->graph(0)->setName("DPM normalized, " + QString::number(power) + " dBm");
                        }else{
                            plot->graph(0)->setName("DPM, " + QString::number(power) + " dBm");
                        }

                        QString freqCmd = "sens:freq ";
                        QString freqStr;
                        freqStr.sprintf("%6.2f",freq*x4*x6); // in GHz, also with 4 and 6 mulipiler, the DPM use GHz
                        freqStr.remove(" ");
                        freqCmd = freqCmd + freqStr;
                        //qDebug() << "-----------" << freqCmd;
                        sprintf(powerMeter->cmd, "%s\n", freqCmd.toStdString().c_str());
                        powerMeter->SendCmd(powerMeter->cmd);

                        //wait for waittime, for the powereter to measure the freq.
                        QEventLoop eventLoop;
                        QTimer::singleShot(waitTime, &eventLoop, SLOT(quit()));
                        eventLoop.exec();

                        sprintf(powerMeter->cmd, "READ?\n");
                        QString readingStr = powerMeter->Ask(powerMeter->cmd);
                        QString unit = readingStr.right(2);
                        //qDebug() << "................." << readingStr << ", unit : " << unit;
                        readingStr.chop(2);
                        readingPM = readingStr.toDouble();
                        //change the unit to mW for other unit.
                        if( unit == "UW" ) readingPM = readingPM / 1000.;

                        //Warning when power > 20 mW;
                        if( readingPM >= 20. ) {
                            LogMsg("##################################################", Qt::red);
                            LogMsg("####### DANGER! power >= 20 mW !!!  ##############", Qt::red);
                            LogMsg("##################################################", Qt::red);
                        }

                    }

                    if( hasDMM ){

                        if( ui->checkBox_Normalize->isChecked()){
                            plot->graph(0)->setName("DMM normalized, " + QString::number(power) + " dBm");
                        }else{
                            plot->graph(0)->setName("DMM, " + QString::number(power) + " dBm");
                        }
                        sprintf(DMM->cmd, ":READ?\n");
                        readingDMM = DMM->Ask(DMM->cmd).toDouble() * 1000;

                        //wait for waittime, for the power meter to measure the freq.
                        QEventLoop eventLoop;
                        QTimer::singleShot(waitTime, &eventLoop, SLOT(quit()));
                        eventLoop.exec();
                    }

                    double reading = 0;
                    if( hasPowerMeter && !hasDMM){
                        reading = readingPM ;
                    }else if( !hasPowerMeter && hasDMM ){
                        reading = readingDMM;
                    }else{
                        reading = 0.;
                    }

                    // fill plots
                    y.push_back(reading / normFactor);
                    z[j-1].push_back(reading); // z always store un-normalized data

                    if( i == 1) {
                        yMin = reading;
                        xMin = freq;
                    }

                    if( yMin > reading ){
                        yMin = reading;
                        xMin = freq;
                    }

                    // plotgraph
                    plot->graph(0)->clearData();
                    plot->graph(0)->setData(x,y);
                    plot->yAxis->rescale();
                    plot->replot();

                    colorMap->data()->setData(freq*multi, power, reading / normFactor);
                    colorMap->rescaleDataRange();
                    auxPlot->replot();


                } // end of for-loop for freqeuncy
               // qDebug() << z[j-1];
            } // end of for-loop for power
        }
        endloop: ;
        if(programMode == 3){
            ui->verticalSlider_power->setMaximum(y2.size());
            ui->verticalSlider_power->setValue(y2.size());
            ui->verticalSlider_power->setEnabled(true);
            ui->checkBox_Normalize->setEnabled(true);
        }

        if( generatorType == SG6000LDQ){
            write2Generator("OUTP:STAT OFF"); // switch off RF
            write2Generator("*BUZZER ON");
        }
        ui->pushButton_Sweep->setStyleSheet("");

        controlOnOFF(true);
        ui->pushButton_ReadPower->setEnabled(true);
        ui->pushButton_RFOnOff->setEnabled(true);
        ui->comboBox_yAxis->setEnabled(true);
        ui->spinBox_Average->setEnabled(true);
        ui->spinBox_Average->setValue(1);

        if( programMode !=4 ){
            QString msg;
            msg.sprintf("Min(x,y) = (%f, %f)", xMin, yMin);
            LogMsg(msg);
        }

        sweepOnOff = false;

    }

    ui->comboBox_yAxis->setEnabled(true);
    ui->spinBox_Average->setEnabled(true);


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

        generatorCount = 0;
        if(info.serialNumber() == "DQ000VJLA" && info.manufacturer() == "FTDI" ){
            generatorPortName = info.portName();
            generatorType = SG6000LDQ;
            generatorCount ++;
        }

        if(info.serialNumber() == "000889" && info.manufacturer() == "Hittite" ){
            //if(info.serialNumber() == "001396" && info.manufacturer() == "Microsoft" ){
            generatorPortName = info.portName();
            generatorType = HMCT2220;
            generatorCount ++;
            if( generatorCount > 1){
                LogMsg(" Two generators found, use HMC-T222O. " , Qt::blue);
            }
        }

    }
    LogMsg ("--------------");
}

void MainWindow::write2Generator(const QString &msg)
{
    const QString temp = msg + "\n";
    generator->write(temp.toStdString().c_str());
    LogMsg("Generator write : " + msg);

    //wait for the serial port to respond.
    QEventLoop eventLoop;
    QTimer::singleShot(1, &eventLoop, SLOT(quit()));
    eventLoop.exec();
}

void MainWindow::readFromGenerator()
{
    QByteArray read = generator->readAll();
    LogMsg("Generator ans = " + QString(read));
    generatorLastRepond = QString(read);
}

void MainWindow::controlOnOFF(bool IO)
{
    ui->lineEdit->setEnabled(IO);
    ui->pushButton_SendCommand->setEnabled(IO);

    ui->doubleSpinBox_Power->setEnabled(IO);
    ui->spinBox_Dwell->setEnabled(IO);
    ui->spinBox_Points->setEnabled(IO);
    ui->lineEdit_Start->setEnabled(IO);
    ui->lineEdit_Stop->setEnabled(IO);

    ui->lineEdit_Multiplier->setEnabled(IO);

    ui->lineEdit_Freq->setEnabled(IO);

    if(generatorType == SG6000LDQ){
        ui->doubleSpinBox->setEnabled(IO);
    }

    if(generatorType == HMCT2220){
        ui->lineEdit_PowerStart->setEnabled(IO);
        ui->lineEdit_PowerEnd->setEnabled(IO);
        ui->spinBox_PowerStep->setEnabled(IO);
    }
}

void MainWindow::on_pushButton_SendCommand_clicked()
{
    write2Generator(ui->lineEdit->text());
}


void MainWindow::on_spinBox_Points_valueChanged(int arg1)
{
    double start = ui->lineEdit_Start->text().toDouble();
    double stop = ui->lineEdit_Stop->text().toDouble();
    double step = (stop-start) / (arg1-1) * 1000.; // in MHz
    double waitTime = ui->spinBox_Dwell->value();
    int powerStep = 1;
    if( generatorType== HMCT2220) powerStep = ui->spinBox_PowerStep->value();
    double runTime = powerStep * arg1 * (waitTime+1) / 1000.;
    ui->lineEdit_StepSize->setText(QString::number(step) + " MHz");
    ui->lineEdit_RunTime->setText(QString::number(runTime) + " sec");
}

void MainWindow::on_spinBox_Dwell_valueChanged(int arg1)
{
    int points = ui->spinBox_Points->value();
    double runTime = points * (arg1+1) / 1000.;
    ui->lineEdit_RunTime->setText("~" + QString::number(runTime) + " sec");
}

void MainWindow::on_lineEdit_Start_textChanged(const QString &arg1)
{
    int multi = ui->lineEdit_Multiplier->text().toInt();
    double effStart = multi * arg1.toDouble();
    ui->lineEdit_EffStart->setText(QString::number(effStart));
    ui->lineEdit_EffStart_d6->setText(QString::number(effStart/x6));

    double stop = ui->lineEdit_Stop->text().toDouble();
    int points = ui->spinBox_Points->value();
    double range = (stop - arg1.toDouble());
    double step = range/(points-1) * 1000.; // in MHz
    ui->lineEdit_StepSize->setText(QString::number(step) + "MHz");

    plot->xAxis->setRange((arg1.toDouble() - range*0.1)*multi, (stop +  range*0.1)*multi);
    plot->replot();

    if(programMode == 3){
        colorMap->data()->setKeyRange(QCPRange( effStart, stop * multi));
        colorMap->rescaleAxes();
        auxPlot->replot();
    }

    checkPowerMeterFreq(arg1.toDouble());
}

void MainWindow::on_lineEdit_Stop_textChanged(const QString &arg1)
{
    double start = ui->lineEdit_Start->text().toDouble();
    int points = ui->spinBox_Points->value();
    double range = (arg1.toDouble() - start);
    double step = range/(points-1) * 1000;
    ui->lineEdit_StepSize->setText(QString::number(step) + " MHz");

    int multi = ui->lineEdit_Multiplier->text().toInt();
    double effStop = multi * arg1.toDouble();
    ui->lineEdit_EffStop->setText(QString::number(effStop));
    ui->lineEdit_EffStop_d6->setText(QString::number(effStop/x6));

    plot->xAxis->setRange((start - range*0.1)*multi, (arg1.toDouble()+  range*0.1)*multi);
    plot->replot();

    if(programMode == 3){
        colorMap->data()->setKeyRange(QCPRange( start*multi, effStop));
        colorMap->rescaleAxes();
        auxPlot->replot();
    }

    checkPowerMeterFreq(arg1.toDouble());
}

void MainWindow::on_doubleSpinBox_Power_valueChanged(double arg1)
{
    if( generatorType == SG6000LDQ){
        write2Generator("PWR " + QString::number(arg1));
    }
    if( generatorType == HMCT2220){
        write2Generator("Power " + QString::number(arg1));
    }
}

void MainWindow::on_doubleSpinBox_valueChanged(double arg1)
{
    write2Generator("ATT " + QString::number(arg1));
}

void MainWindow::on_actionSave_Data_triggered()
{
    int size = x.size();

    if( x.size() == 0){
        LogMsg("no data to save.", Qt::blue);
        return;
    }

    if(programMode == 1){
        if( x.size() != y.size()) {
            LogMsg("data corrupt. Please measure again.", Qt::red);
            return;
        }
    }
    if( programMode == 2){
        if( x.size() != y.size() || x.size() != y2.size()) {
            LogMsg("data corrupt. Please measure again.", Qt::red);
            return;
        }
    }

    QDateTime date = QDateTime::currentDateTime();
    QString fileName = date.toString("yyyyMMdd_HHmmss") + "_DNP100GHz.dat";
    QString filePath = DATA_PATH + fileName;

    QFileDialog fileDialog(this);
    fileDialog.setDirectory("Z:/triplet/");
    filePath = fileDialog.getSaveFileName(this, "Save to", filePath);

    QFile outfile(filePath);

    outfile.open(QIODevice::WriteOnly);

    QTextStream stream(&outfile);
    QString lineout;

    lineout.sprintf("###%s", date.toString("yyyy-MM-dd HH:mm:ss\n").toStdString().c_str()); stream << lineout;
    lineout.sprintf("###Program mode %d", programMode); stream << lineout;
    if( programMode !=3 ){
        lineout.sprintf("###number of data = %d\n", size); stream << lineout;
        if(programMode == 2){
            lineout.sprintf("%10s\t%10s\t%10s\n", "freq.[GHz]", "DPM [mW]", "DMM [mV]"); stream << lineout;
        }
        if( programMode == 1){
            if( hasPowerMeter && !hasDMM ){
                lineout.sprintf("###Using Digital Power Meter."); stream << lineout;
                lineout.sprintf("%10s\t%10s\n", "freq.[GHz]", "DPM [mW]"); stream << lineout;
            }
            if( !hasPowerMeter && hasDMM ){
                lineout.sprintf("###Using Digital Multi-Meter."); stream << lineout;
                lineout.sprintf("%10s\t%10s\n", "freq.[GHz]", "DMM [mV]"); stream << lineout;
            }
        }


        for( int i = 0; i < size; i++){
            if( programMode == 2){
                lineout.sprintf("%10f\t%10f\t%10f\n", x[i], y[i], y2[i]); stream << lineout;
            }else{
                lineout.sprintf("%10f\t%10f\n", x[i], y[i]); stream << lineout;
            }
        }

    }else{
        lineout.sprintf("### number of freq [GHz] = %d\n", size); stream<< lineout;
        lineout.sprintf("### number of power [dBm] = %d\n", y2.size()); stream<< lineout;

        //first line
        lineout.sprintf("#%13s,", "freq[GHz]"); stream << lineout;
        for(int i = 0; i < size - 1; i++){
            lineout.sprintf("%10.f, ", x[i]); stream << lineout;
        }
        lineout.sprintf("%10.f\n", x[size-1]); stream << lineout;

        //date line
        for(int j = 0; j < y2.size()-1; j++){
            lineout.sprintf(" Power=%7.3f,", y2[j]); stream << lineout;
            for(int i = 0; i < size -1; i++){
                lineout.sprintf("%10.f, ", z[j][i]); stream << lineout;
            }
            lineout.sprintf("%10.f\n", z[j][size-1]); stream << lineout;
        }
    }

    stream << "========= end of file =======";
    outfile.close();

    LogMsg("saved data to : " + fileName);
}

void MainWindow::on_pushButton_ReadPower_clicked()
{
    if( hasPowerMeter ){
        LogMsg("====== Read power from DPM.");
        sprintf(powerMeter->cmd, "sens:freq?\n");
        powerMeter->Ask(powerMeter->cmd);

        sprintf(powerMeter->cmd, "read?\n");
        QString readingStr = powerMeter->Ask(powerMeter->cmd);
        QString unit = readingStr.right(2);
        readingStr.chop(2);
        double reading = readingStr.toDouble();
        //change the unit to mW for other unit.
        if( unit == "UW" ) reading = reading / 1000.;

        if( reading >= 20. ) {
            LogMsg("##################################################", Qt::red);
            LogMsg("####### DANGER! power >= 20 mW !!!  ##############", Qt::red);
            LogMsg("##################################################", Qt::red);
        }
    }

    if( hasDMM ){
        LogMsg("====== Read power from DMM.");
        sprintf(DMM->cmd, ":READ?\n");
        DMM->Ask(DMM->cmd);
    }
}

void MainWindow::on_actionSave_plot_triggered()
{
    if( x.size() == 0){
        LogMsg("no data to save.");
        return;
    }

    if( x.size() != y.size()) {
        LogMsg("data corrupt. Please measure again.");
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
    rfOnOff = !rfOnOff;

    if(rfOnOff){
        ui->pushButton_Sweep->setEnabled(false);
        ui->pushButton_RFOnOff->setStyleSheet("background-color: rgb(0,255,0)");
        double freq = ui->lineEdit_Freq->text().toDouble();
        double multi = ui->lineEdit_Multiplier->text().toDouble();
        if(generatorType == SG6000LDQ) {
            QString inputStr;
            inputStr.sprintf("FREQ:CW %fMHz", freq*1000);
            write2Generator(inputStr);
        }
        if(generatorType == HMCT2220){
            write2Generator("FREQ " + QString::number(freq*1000*multi/x6)+" MHz");
        }
        write2Generator("OUTP:STAT ON");

        if( generatorType == HMCT2220){
            double power = ui->doubleSpinBox_Power->value();
            write2Generator("Power " + QString::number(power));
        }

        controlOnOFF(false);
        ui->doubleSpinBox_Power->setEnabled(true);
        ui->lineEdit_Freq->setEnabled(true);
        ui->lineEdit->setEnabled(true);
        ui->pushButton_SendCommand->setEnabled(true);
        if( generatorType == SG6000LDQ){
            ui->doubleSpinBox->setEnabled(true);
        }

        //Set DPM freq;
        if(hasPowerMeter){
            QString freqCmd = "sens:freq ";
            QString freqStr;
            freqStr.sprintf("%6.2f",freq*x4*x6); // in GHz, also with 4 and 6 mulipiler.
            freqStr.remove(" ");
            freqCmd = freqCmd + freqStr;
            //qDebug() << "-----------" << freqCmd;
            sprintf(powerMeter->cmd, "%s\n", freqCmd.toStdString().c_str());
            powerMeter->SendCmd(powerMeter->cmd);
        }
    }else{
        ui->pushButton_RFOnOff->setStyleSheet("");
        write2Generator("OUTP:STAT OFF");
        controlOnOFF(true);
        ui->pushButton_Sweep->setEnabled(true);
    }

}

void MainWindow::on_lineEdit_Multiplier_textChanged(const QString &arg1)
{
    double effStart = arg1.toInt() * ui->lineEdit_Start->text().toDouble();
    double effStop = arg1.toInt() * ui->lineEdit_Stop->text().toDouble();
    double effFreq = arg1.toInt() * ui->lineEdit_Freq->text().toDouble();

    ui->lineEdit_EffStart->setText(QString::number(effStart));
    ui->lineEdit_EffStop->setText(QString::number(effStop));
    ui->lineEdit_EffFreq->setText(QString::number(effFreq));  

    double start = ui->lineEdit_Start->text().toDouble();
    double stop = ui->lineEdit_Stop->text().toDouble();
    double range = stop - start;

    int multi = arg1.toInt();

    plot->xAxis->setRange((start - range*0.1)*multi, (stop +  range*0.1)*multi);
    plot->replot();
}

void MainWindow::on_lineEdit_Freq_textChanged(const QString &arg1)
{
    int multi = ui->lineEdit_Multiplier->text().toInt();
    double effFreq = multi * arg1.toDouble();
    ui->lineEdit_EffFreq->setText(QString::number(effFreq));
}

void MainWindow::on_comboBox_yAxis_currentIndexChanged(int index)
{
    qDebug() << "index : " << index << " ," << ui->comboBox_yAxis->itemText(index) ;

    if( x.isEmpty()) return;

    if( programMode == 2 ) return;

    ui->checkBox_Normalize->setChecked(false);
    on_checkBox_Normalize_clicked(false);

    plot->graph(0)->clearData();
    plot->graph(0)->addData(x, y);
    plot->yAxis->setLabel("Power [mW]");

    if(hasPowerMeter){
        double yMax = 0, yMin = 0;
        for(int i = 0; i < y.size(); i++){
            if(yMax < y[i]) yMax = y[i];
            if(yMin > y[i]) yMin = y[i];
        }
        qDebug() << yMin << "," << yMax;

        if(index == 0){
            plot->yAxis->setScaleType(QCPAxis::stLinear);
            LogMsg("DPM changes to linear-y");
        }else if( index == 1){
            plot->yAxis->setScaleType(QCPAxis::stLogarithmic);
            plot->yAxis->setRange(yMin, yMax);
            LogMsg("DPM changes to log-y");
        }else{
            // cal dB
            dB.clear();
            for(int i = 0; i < y.size(); i++){
                dB.push_back( 10 * qLn(y[i]/yMax)/qLn(10));
            }
            plot->yAxis->setScaleType(QCPAxis::stLinear);
            plot->graph(0)->clearData();
            plot->graph(0)->addData(x, dB);
            plot->yAxis->setLabel("power [dB]");
            LogMsg("DPM changes to dB-y");
        }

        plot->yAxis->rescale();
        plot->replot();
    }

    if(hasDMM){
        double yMax = 0, yMin = 0;
        for(int i = 0; i < y.size(); i++){
            if(yMax < y[i]) yMax = y[i];
            if(yMin > y[i]) yMin = y[i];
        }
        qDebug() << yMin << "," << yMax;

        if(index == 0){
            plot->yAxis->setScaleType(QCPAxis::stLinear);
            LogMsg("DMM changes to linear-y");
        }else if( index == 1){
            plot->yAxis->setScaleType(QCPAxis::stLogarithmic);
            plot->yAxis->setRange(yMin, yMax);
            LogMsg("DMM changes to log-y");
        }else{
            // cal dB2
            dB2.clear();
            for(int i = 0; i < y.size(); i++){
                dB2.push_back( 10 * qLn(y[i]/yMax)/qLn(10));
            }
            plot->yAxis->setScaleType(QCPAxis::stLinear);
            plot->graph(0)->clearData();
            plot->graph(0)->addData(x, dB2);
            plot->yAxis->setLabel("power [dB]");
            LogMsg("DMM changes to dB-y");
        }

        plot->yAxis->rescale();
        plot->replot();
    }

}

void MainWindow::on_spinBox_Average_valueChanged(int arg1)
{
    if( x.isEmpty()) return;
    if(programMode == 2) return;

    int index = ui->comboBox_yAxis->currentIndex();

    if( arg1 == 1){
        on_comboBox_yAxis_currentIndexChanged(index);
        return;
    }

    QVector<double> ax, ay, adB;
    int iHalf = (arg1 -1)/2;
    int size = x.size();
    int iStop = size - 1 - iHalf;
    for( int i = iHalf; i <= iStop; i++){
        ax.push_back(x[i]);
        double yAvg = 0;
        for(int j = -iHalf; j < iHalf; j++){
            yAvg += y[i+j]/arg1;
        }
        ay.push_back(yAvg);
    }
    plot->graph(0)->clearData();
    plot->graph(0)->addData(ax, ay);

    if( index == 2){
        double yMax = *std::max_element(y.begin(), y.end());
        for( int i = 0; i <= ay.size(); i++){
            adB.push_back( 10 * qLn(ay[i]/yMax)/qLn(10));
        }
        plot->graph(0)->clearData();
        plot->graph(0)->addData(ax, adB);
    }
    plot->yAxis->rescale();
    plot->replot();

}

void MainWindow::SetSwitchMatrixPort(QString slot, int port)
{
    if(!switchConnected) return;
    if( switchMatrix->dynamicCall("Set_Switch(QString&, int&)", slot, port).toInt() == 0){
        LogMsg("Error when setting switch");
    }
}

void MainWindow::on_horizontalSlider_A_valueChanged(int value)
{
    if(!switchConnected) return;
    LogMsg("Set SwitchName A to port " + QString::number(value+1));
    SetSwitchMatrixPort("A", value);
    if( ui->checkBox_Sync->isChecked()){
        ui->horizontalSlider_B->setValue(value);
    }
}

void MainWindow::on_horizontalSlider_B_valueChanged(int value)
{
    if(!switchConnected) return;
    LogMsg("Set SwitchName B to port " + QString::number(value+1));
    SetSwitchMatrixPort("B", value);
}

void MainWindow::checkPowerMeterFreq(double freq)
{
    double DPMfreq = freq * x4*x6 ; // in GHz with 4x + 6x multiplier
    if( DPMfreq < 75 || DPMfreq > 110){
        LogMsg("##################################################", Qt::red);
        LogMsg("The freq in the DPM : " + QString::number(DPMfreq) + " GHz.", Qt::red);
        LogMsg("Out of the range of the DPM!", Qt::red);
        LogMsg("##################################################", Qt::red);
    }
}

void MainWindow::on_pushButton_GetNoPoint_clicked()
{
    sprintf(powerMeter->cmd, "calc:aver:coun?\n");
    int ans = (powerMeter->Ask(powerMeter->cmd)).toInt();
    ui->spinBox_AveragePoints->setValue(ans);
    LogMsg(" DPM no. of points : " + QString::number(ans) + ".");
}

void MainWindow::on_spinBox_AveragePoints_editingFinished()
{
    sprintf(powerMeter->cmd, "calc:aver:coun %d", ui->spinBox_AveragePoints->value());
    powerMeter->SendCmd(powerMeter->cmd);
}

void MainWindow::on_lineEdit_PowerStart_textChanged(const QString &arg1)
{
    double end = ui->lineEdit_PowerEnd->text().toDouble();
    int step = ui->spinBox_PowerStep->value();

    if( arg1.toDouble() == end){
        step = 1;
        ui->spinBox_PowerStep->setValue(1);
    }

    double stepSize = (end-arg1.toDouble())/(step-1);
    ui->lineEdit_PowerStepSize->setText(QString::number(stepSize) + " dBm");

    if(programMode == 3){
        colorMap->data()->setValueRange(QCPRange( end, arg1.toDouble()));
        colorMap->rescaleAxes();
        auxPlot->replot();
    }
}

void MainWindow::on_lineEdit_PowerEnd_textChanged(const QString &arg1)
{
    double start = ui->lineEdit_PowerStart->text().toDouble();
    int step = ui->spinBox_PowerStep->value();

    if( arg1.toDouble() == start){
        step = 1;
        ui->spinBox_PowerStep->setValue(1);
    }

    double stepSize = (arg1.toDouble()-start)/(step-1);
    ui->lineEdit_PowerStepSize->setText(QString::number(stepSize) + " dBm");

    if(programMode == 3){
        colorMap->data()->setValueRange(QCPRange( arg1.toDouble(), start ));
        colorMap->rescaleAxes();
        auxPlot->replot();
    }
}

void MainWindow::on_spinBox_PowerStep_valueChanged(int arg1)
{
    double start = ui->lineEdit_PowerStart->text().toDouble();
    double end = ui->lineEdit_PowerEnd->text().toDouble();

    double stepSize = (end-start)/(arg1-1);
    ui->lineEdit_PowerStepSize->setText(QString::number(stepSize) + " dBm");

    double waitTime = ui->spinBox_Dwell->value();
    int step = ui->spinBox_Points->value();
    double runTime = step * arg1 * (waitTime+1) / 1000.;

    ui->lineEdit_RunTime->setText(QString::number(runTime) + " sec");
}

void MainWindow::on_actionSave_2_D_plot_triggered()
{
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
    int ph = auxPlot->geometry().height();
    int pw = auxPlot->geometry().width();
    if( fileDialog.selectedNameFilter() == nameFilterList[0]){
        if( fileName.right(4) != ".pdf" ) fileName.append(".pdf");
        ok = auxPlot->savePdf(fileName, pw, ph );
    }else{
        if( fileName.right(4) != ".png" ) fileName.append(".png");
        ok = auxPlot->savePng(fileName, pw, ph );
    }

    if( ok ){
        LogMsg("Saved 2D-Plot as " + fileName);
    }else{
        LogMsg("Save Failed.");
    }
}

void MainWindow::on_checkBox_Normalize_clicked(bool checked)
{
    if( programMode != 3 ) return;
    if(checked){
        if( hasPowerMeter ) {
            colorScale->axis()->setLabel("DPM power / input power");
            plot->yAxis->setLabel("DPM power / input power");
            plot->graph(0)->setName("DPM normalized");
        }
        if( hasDMM ) {
            colorScale->axis()->setLabel("DMM power / input power [mV/mW]");
            plot->yAxis->setLabel("DMM power / input power [mV/mW]");
            plot->graph(0)->setName("DMM normalized");
        }

    }else{
        if( hasPowerMeter ) {
            colorScale->axis()->setLabel("DPM power [mW]");
            plot->yAxis->setLabel("DPM power [mW]");
            plot->graph(0)->setName("DPM");
        }
        if( hasDMM ) {
            colorScale->axis()->setLabel("DMM power [mV]");
            plot->yAxis->setLabel("DMM power [mW]");
            plot->graph(0)->setName("DMM");
        }
    }

    // if data is not empty, plot again

    if(!x.isEmpty()){

        double normFactor = 1;

        for(int j = 0; j < y2.size() ; j++){
            if( ui->checkBox_Normalize->isChecked()){
                normFactor = dBm2mW(y2[j]);
            }
            for(int i = 0; i < x.size() ; i++){
                colorMap->data()->setData(x[i], y2[j], z[j][i] / normFactor);
            }
        }
        colorMap->data()->recalculateDataBounds();
        colorMap->rescaleDataRange();
        //double zMax = colorMap->dataRange().upper;
        //colorScale->setDataRange(QCPRange(0, zMax));

        auxPlot->replot();

        int y2Index = ui->verticalSlider_power->value()-1;
        y.clear();
        if( ui->checkBox_Normalize->isChecked()){
            normFactor = dBm2mW(y2[y2Index]);
        }
        for(int i = 0; i < x.size() ; i++){
            y.push_back(z[y2Index][i] /  normFactor);
        }

        plot->graph(0)->clearData();
        plot->graph(0)->setData(x,y);
        plot->yAxis->rescale();
        plot->replot();
    }

    plot->replot();
    auxPlot->replot();
}

void MainWindow::on_lineEdit_dBm_textChanged(const QString &arg1)
{
    if(stopdBm2mW) return;
    stopmW2dBm = true;
    double dBm = arg1.toDouble();
    double mW = dBm2mW(dBm);
    ui->lineEdit_mW->setText(QString::number(mW));
    stopmW2dBm = false;
}

void MainWindow::on_lineEdit_mW_textChanged(const QString &arg1)
{
    if(stopmW2dBm) return;
    stopdBm2mW = true;
    double mW = arg1.toDouble();
    double dBm = mW2dBm(mW);
    ui->lineEdit_dBm->setText(QString::number(dBm));
    stopdBm2mW = false;
}

void MainWindow::on_lineEdit_Freq_returnPressed()
{
    double freq = ui->lineEdit_Freq->text().toDouble();
    double multi = ui->lineEdit_Multiplier->text().toDouble();
    if( generatorType == SG6000LDQ){
        QString inputStr;
        inputStr.sprintf("FREQ:CW %fMHz", freq*1000.); // in MHz
        write2Generator(inputStr);
    }
    if( generatorType == HMCT2220){
        write2Generator("FREQ " + QString::number(freq* 1000* multi/x6)+"MHz");
    }
}

void MainWindow::on_verticalSlider_power_valueChanged(int value)
{
    if( !ui->verticalSlider_power->isEnabled() ) return;
    if( x.isEmpty()) {
        LogMsg("no data" , Qt::red);
        return;
    }

    // a trick to stop on_combBox_yAxis_currentIndexChanged()
    int old_programMode = programMode;
    programMode = 2;
    ui->comboBox_yAxis->setCurrentIndex(0);
    programMode = old_programMode;

    //if( y2.size() != z->size()) {
    //    LogMsg("data crrupted" , Qt::red);
    //    return;
    //}

    // fill y with z[value-1]
    double normFactor = 1;
    if( ui->checkBox_Normalize->isChecked()){
        normFactor = dBm2mW(y2[value-1]);
    }
    y.clear();
    for(int i = 0; i < x.size() ; i++){
        y.push_back(z[value-1][i] / normFactor);
    }

    // change plot name
    double power = y2[value-1];
    if( hasPowerMeter){
        if( ui->checkBox_Normalize->isChecked()){
            plot->graph(0)->setName("DPM normalized, " + QString::number(power) + " dBm");
        }else{
            plot->graph(0)->setName("DPM, " + QString::number(power) + " dBm");
        }
    }
    if(hasDMM){
        if( ui->checkBox_Normalize->isChecked()){
            plot->graph(0)->setName("DMM normalized, " + QString::number(power) + " dBm");
        }else{
            plot->graph(0)->setName("DMM, " + QString::number(power) + " dBm");
        }
    }

    // plotgraph
    plot->graph(0)->clearData();
    plot->graph(0)->setData(x,y);
    plot->yAxis->rescale();
    plot->replot();

}

void MainWindow::on_checkBox_Sync_clicked(bool checked)
{
    int valueA = ui->horizontalSlider_A->value();
    if(checked){
        ui->horizontalSlider_B->setEnabled(false);
        ui->horizontalSlider_B->setValue(valueA);
    }else{
        ui->horizontalSlider_B->setEnabled(true);
    }
}

void MainWindow::on_actionHelp_Page_triggered()
{
    if( helpDialog->isHidden() ){
        helpDialog->show();
    }
}
