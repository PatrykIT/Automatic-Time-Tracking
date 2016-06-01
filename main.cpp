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
 * 1. Make strong log files. Write after each iteration each container's values, output of ps and wmctrl output.
 * 2. Add error codes / throwing if regex fails.
*/
