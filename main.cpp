#include "mainwindow.h"

#include <QApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}

/*
 * 1. Manager.cpp line 250. Add start counting time when app was restarted. Test if all works.
 * 2. Add error codes / throwing if regex fails.
*/
