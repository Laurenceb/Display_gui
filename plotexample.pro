#-------------------------------------------------
#
# Project created by QtCreator 2016-03-17T17:44:45
#
#-------------------------------------------------

QT       += core gui
QT += serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = plotexample
TEMPLATE = app


SOURCES += qcustomplot.cpp \
    tabdialog.cpp \
    graph.cpp \
    main.cpp \
    window.cpp \
    serialport.cpp \
    portselectdialog.cpp \
    connectionmanager.cpp \
    fileselectdialog.cpp \
    BPMestimator.cpp

HEADERS  += qcustomplot.h \
    tabdialog.h \
    graph.h \
    window.h \
    portselectdialog.h \
    serialport.h \
    connectionmanager.h \
    fileselectdialog.h \
    BPMestimator.h

FORMS += \
    mainwindow.ui
