#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QCloseEvent>

#include <memory>


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
    std::unique_ptr<QGraphicsScene> graphic_scene;


private:
    void closeEvent(QCloseEvent *event);

private slots:
    void on_Start_Stats_Button_clicked();
    //void Show_Icon(const QPixmap icon); //TO DO: Can we make it private again?
public slots:
    void Show_Icon(const QPixmap icon); //Arguments are always copied when you have a queued connection, so no need for reference.
};

#endif // MAINWINDOW_H
