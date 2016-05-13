#-------------------------------------------------
#
# Project created by QtCreator 2016-02-27T13:39:32
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ManicLTime
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    Item.cpp \
    Manager.cpp

HEADERS  += mainwindow.h \
    Item.h \
    Manager.h

FORMS    += mainwindow.ui

CONFIG += c++11

QMAKE_CXXFLAGS += -std=c++0x
