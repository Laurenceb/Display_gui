#-------------------------------------------------
#
# Project created by QtCreator 2016-03-17T17:44:45
#
#-------------------------------------------------

QT       += core gui
QT += serialport
QT += bluetooth

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = plotexample
TEMPLATE = app
RC_FILE = myapp.rc

win32 {
  QMAKE_LFLAGS += -static-libgcc -static-libstdc++
}

SOURCES += qcustomplot.cpp \
    tabdialog.cpp \
    graph.cpp \
    main.cpp \
    window.cpp \
    serialport.cpp \
    portselectdialog.cpp \
    connectionmanager.cpp \
    fileselectdialog.cpp \
    BPMestimator.cpp \
    bluetooth.cpp

HEADERS  += qcustomplot.h \
    tabdialog.h \
    graph.h \
    window.h \
    portselectdialog.h \
    serialport.h \
    connectionmanager.h \
    fileselectdialog.h \
    BPMestimator.h \
    bluetooth.h

FORMS += \
    mainwindow.ui
