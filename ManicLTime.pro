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

HEADERS  += mainwindow.h \
    Item.h \
    Manager.h \

FORMS    += mainwindow.ui

CONFIG += c++14
