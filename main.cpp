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
 * 1. Finished on Stop_Counting_Time(). Check if Add_Item() works, and test Start() function. Then go ahead with the rest of app :)
 * 2. Add error codes / throwing if regex fails.
*/
