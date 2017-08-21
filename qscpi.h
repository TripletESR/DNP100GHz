#ifndef QSCPI_H
#define QSCPI_H

#include <QObject>
#include <stdio.h>
#include <cmath>
#include <windows.h>
//#include "C:\Program Files (x86)\IVI Foundation\VISA\WinNT\Include\visa.h"
#include "C:\Program Files\IVI Foundation\VISA\Win64\Include\visa.h"
#include <QDebug>

class QSCPI : public QObject
{
    Q_OBJECT
public:
    char cmd[100], buf[256];
    ViStatus status;
    QString name, scpi_Msg;

    //explicit QSCPI(ViRsrc name,QObject *parent = 0);
    explicit QSCPI(ViRsrc name);
    ~QSCPI();

signals:

    void SendMsg(QString msg);
    void DeviceNotReady(double time);
    void DeviceReady(QString msg);

public slots:

    void Reset();
    void Clear();

    QString GetName();

    QString GetErrorMsg();
    void SendCmd(char *cmd);
    QString ReadRespond();
    QString Ask(char *cmd);
    bool isOperationCompleted();

    int StatusByteRegister();
    int EventStatusRegister();

protected:
    ViSession defaultRM;
    ViSession device;
};

#endif // QSCPI_H
