#-------------------------------------------------
#
# Project created by QtCreator 2014-06-23T15:42:30
#
#-------------------------------------------------

QT       += core gui serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = hostcomm
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    tdt4255board.cpp \
    QHexEdit/commands.cpp \
    QHexEdit/qhexedit.cpp \
    QHexEdit/qhexedit_p.cpp \
    QHexEdit/xbytearray.cpp

HEADERS  += mainwindow.h \
    tdt4255board.h \
    QHexEdit/commands.h \
    QHexEdit/qhexedit.h \
    QHexEdit/qhexedit_p.h \
    QHexEdit/xbytearray.h

INCLUDEPATH += QHexEdit

FORMS    += mainwindow.ui
