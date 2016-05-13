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
 * 1. Check if ui is copied good. We need to have access to it from Manager class. If good, then finisz Manager::Start()
 * 2. Add error codes / throwing if regex fails.
*/
