#include "qscpi.h"

QSCPI::QSCPI(ViRsrc name)
{
    qDebug() << "--------------------------------------";
    scpi_Msg.sprintf("======================================\n"
                "Opening SCPI device : %s", name);
    //emit signal did not work, because no connection is made yet
    //emit SendMsg(scpi_Msg);
    viOpenDefaultRM(&defaultRM);
    status = viOpen(defaultRM, name, VI_NULL, VI_NULL, &device);
    Clear();
}
QSCPI::~QSCPI(){
    viClose(device);
    viClose(defaultRM);
    //qDebug() << "Closing VIAS session : " << this->name;
    scpi_Msg.sprintf("Closing VIAS session : %s", this->name.toStdString().c_str());
    emit SendMsg(scpi_Msg);
}

void QSCPI::SendCmd(char *cmd){
    if( status != VI_SUCCESS ) return;
    viPrintf(device, cmd);
    *std::remove(cmd, cmd+strlen(cmd), '\n') = '\0'; //remove "\n"
    //qDebug("%s", cmd);
    scpi_Msg.sprintf("%s", cmd);
    emit SendMsg(scpi_Msg);
}

QString QSCPI::ReadRespond() //Change to AskQ
{
    if( status != VI_SUCCESS ) return "Err.";
    viScanf(device, "%t", this->buf);
    *std::remove(buf, buf+strlen(buf), '\n') = '\0';
    //qDebug("%s", buf);
    scpi_Msg.sprintf("%s ; %#x" , buf, StatusByteRegister());
    emit SendMsg(scpi_Msg);
    return QString(buf);
}

QString QSCPI::Ask(char *cmd){
    if( status != VI_SUCCESS) return "Err.";
    SendCmd(cmd);
    QString res = ReadRespond();
    //scpi_Msg.sprintf("%s" , res.toStdString().c_str());
    //emit SendMsg(scpi_Msg);
    return res;
}

void QSCPI::Reset(){
    if( status != VI_SUCCESS ) return;
    viPrintf(device, "*RST\n");
    //qDebug() << "Reset : " << this->name;
    scpi_Msg.sprintf("Reset device : %s", this->name.toStdString().c_str());
    emit SendMsg(scpi_Msg);
}

void QSCPI::Clear(){
    if( status != VI_SUCCESS ) return;
    viPrintf(device, "*CLS\n");
    //qDebug() << "Clear : " << this->name;
    scpi_Msg.sprintf("Clear device : %s", this->name.toStdString().c_str());
    emit SendMsg(scpi_Msg);
}

QString QSCPI::GetName()
{
    if( status != VI_SUCCESS ) return "";
    sprintf(cmd,"*IDN?\n");
    return Ask(cmd);
}

QString QSCPI::GetErrorMsg(){
    if( status != VI_SUCCESS ) return "Err.";
    sprintf(cmd,"SYST:ERR?\n");
    return Ask(cmd);
}

bool QSCPI::isOperationCompleted(){
    if( status != VI_SUCCESS) return "Err.";
    sprintf(cmd,"*OPC?\n");
    return Ask(cmd).toInt();
}

int QSCPI::StatusByteRegister()
{
    ViUInt16 statusByte;
    viReadSTB(device,&statusByte);
    //scpi_Msg.sprintf("Device Status Byte Register : %#x , %s", statusByte, this->name.toStdString().c_str());
    //emit SendMsg(scpi_Msg);
    return statusByte;
}

int QSCPI::EventStatusRegister()
{
    sprintf(cmd,"*ESR?\n");
    return Ask(cmd).toInt();
}
