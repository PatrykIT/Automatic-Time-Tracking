#ifndef QDROPBOXJSONTEST_H
#define QDROPBOXJSONTEST_H

#include <QtTest>
#include <QDesktopServices>
#include <QThread>

#include "../src/qtdropbox.h"
#include "keys.hpp"

class QtDropboxTest : public QObject
{
    Q_OBJECT

public:
   QtDropboxTest();

private Q_SLOTS:

   /* QDropboxJson */
    void jsonCase1();
    void jsonCase2();
    void jsonCase3();
    void jsonCase4();
    void jsonCase5();
    void jsonCase6();
    void jsonCase7();
    void jsonCase8();
    void jsonCase9();
    void jsonCase10();
    void jsonCase11();
    void jsonCase12();
    void jsonCase13();
    void jsonCase14();
    void jsonCase15();

  /* QDropbox */
    void dropboxCase1();
    void dropboxCase2();

    /* My tests */
    void QDropboxFile_Example();
    void Upload_File();

private:
    void authorizeApplication(QDropbox *dropbox);
    bool connectDropbox(QDropbox* dropbox_object, QDropbox::OAuthMethod method);
};

#endif // QDROPBOXJSONTEST_H
