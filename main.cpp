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
 * 1. Finish Load_Statistics_from_File()
 * 2. Add error codes / throwing if regex fails.
*/
