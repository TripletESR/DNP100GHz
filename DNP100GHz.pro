#-------------------------------------------------
#
# Project created by QtCreator 2017-08-18T17:40:45
#
#-------------------------------------------------

QT       += core gui serialport axcontainer

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

TARGET = DNP100GHz
TEMPLATE = app

#have to use winNT because the compiler WinGW is 32
#win32: LIBS += "C:/Program Files (x86)/IVI Foundation/VISA/WinNT/lib/msc/visa32.lib"
win32: LIBS += "C:/Program Files (x86)/IVI Foundation/VISA/WinNT/agvisa/lib/msc/visa32.lib"

INCLUDEPATH += "C:/Program Files (x86)/IVI Foundation/VISA/WinNT/agvisa/agbin"

win32:RC_ICONS += icon.ico

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        main.cpp \
        mainwindow.cpp\
        qcustomplot.cpp \
    qscpi.cpp

HEADERS += \
        mainwindow.h\
        qcustomplot.h \
    qscpi.h

FORMS += \
        mainwindow.ui
