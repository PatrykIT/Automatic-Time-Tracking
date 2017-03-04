#-------------------------------------------------
#
# Project created by QtCreator 2016-02-27T13:39:32
#
#-------------------------------------------------

QT       += core gui
QT += network
QT += xml
QT += testlib

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ManicLTime
TEMPLATE = app

SOURCES += main.cpp\
        mainwindow.cpp \
    Item.cpp \
    Manager.cpp \
    qtdropbox/QtDropbox/src/qdropboxjson.cpp \
    qtdropbox/QtDropbox/src/qdropboxfileinfo.cpp \
    qtdropbox/QtDropbox/src/qdropboxfile.cpp \
    qtdropbox/QtDropbox/src/qdropboxdeltaresponse.cpp \
    qtdropbox/QtDropbox/src/qdropboxaccount.cpp \
    qtdropbox/QtDropbox/src/qdropbox.cpp \
    qtdropbox/QtDropbox/tests/qtdropboxtest.cpp

HEADERS  += mainwindow.h \
    Item.h \
    Manager.h \
    qtdropbox/QtDropbox/src/qtdropbox_global.h \
    qtdropbox/QtDropbox/src/qtdropbox.h \
    qtdropbox/QtDropbox/src/qdropboxjson.h \
    qtdropbox/QtDropbox/src/qdropboxfileinfo.h \
    qtdropbox/QtDropbox/src/qdropboxfile.h \
    qtdropbox/QtDropbox/src/qdropboxdeltaresponse.h \
    qtdropbox/QtDropbox/src/qdropboxaccount.h \
    qtdropbox/QtDropbox/src/qdropbox.h \
    qtdropbox/QtDropbox/tests/qtdropboxtest.hpp \
    qtdropbox/QtDropbox/tests/keys.hpp

FORMS    += mainwindow.ui

CONFIG += c++14

QMAKE_CXXFLAGS += -std=c++0x
