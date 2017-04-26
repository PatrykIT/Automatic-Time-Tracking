//Local includes
#include "mainwindow.h"
#include "Manager.h"
//#include "qtdropbox/QtDropbox/tests/qtdropboxtest.hpp"


//QT includes
#include <QApplication>
#include <QDebug>

//C++ includes
#include <thread>


//QTEST_MAIN(QtDropboxTest)

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    //std::cout << __PRETTY_FUNCTION__ << std::endl;
    std::unique_ptr<Manager> manager(new Manager);

    /* BlockingQueuedConnection - Same as QueuedConnection, except that the current thread blocks until the slot has been delivered. */
    QObject::connect(manager.get(), &Manager::Show_Icon, &w, &MainWindow::Show_Icon, Qt::BlockingQueuedConnection);/* TO DO: Wouldn't QueuedConnection be better? */

    /* Disconnect logic of program from GUI */
    std::thread manager_thread(&Manager::Start, std::move(manager));
    manager_thread.detach();

    return a.exec();
}

/* TO DO:
 * 1. Change pids to std::unordered_set
 * 2. Add error codes / throwing if regex fails.
 * 3. Implement checking which websites user has visited, and how many time he spent there.
 * 4. Implement checking whether application was active, or inactive(ie. just in background, the user wasn't using it). So there would be 2 statistics:
 *      a) All time the application was ON      b) Time application was active, when the user was using it.
 * 5. Save statistics in a cloud (user loges in to ie. Gmail, so we must have login box etc).
 * 7. Consider changing containers from objects to pointers(smart ptrs would be best). Pros: When lots of copying happens, its faster. Cons: We lose locality of reference.
 * 8. If our program can't find an icon on the disk, maybe we should download it from internet? Nice.
 * 9. We should be making backup copies once for ie. 2 weeks.
 * 11. Change QPixmap to QImage, and then convert to QPixmap. From documentation:
 * "Typically, the QImage class is used to load an image file, optionally manipulating the image data, before the QImage object is converted into a QPixmap to be shown on screen."
 *
 *12. Logger class (debugging info for developer) should be nice - read about best design pattern for it. There should be a hierarchy - some logs are critical,
 * so they should be saved always. It can be done as a simple extra int argument (for now at least).
 *
 * KNOWN BUGS:
 * wmctrl doesn't see Steam running. It outputs: 0 N/A
 *
 *
 *
 * Refactoring to proper OOP:
 * 1. We should have a class Timer. Manager would use it, instead of implementing whole functionality.
 * 2. We should have a class Logs. It would do logs based on priority.
 * 3. Change Manager to Abstract Factory so I can erase #ifdef's everywhere.
 *      So there would be Manager_Linux and Manager_Windows. Both would have the same interfaces, but different implementations.

*/
