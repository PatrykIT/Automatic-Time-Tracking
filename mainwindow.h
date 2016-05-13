#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>


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

private slots:
    void on_Start_Stats_Button_clicked();

private:
    Ui::MainWindow *ui;
    QGraphicsScene *graphic_scene;

};

#endif // MAINWINDOW_H
