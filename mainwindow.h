#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QCloseEvent>


namespace Ui
{
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    QGraphicsScene *graphic_scene;


private:
    void closeEvent(QCloseEvent *event);

private slots:
    void on_Start_Stats_Button_clicked();

};

#endif // MAINWINDOW_H
