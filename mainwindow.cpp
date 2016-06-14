//Local includes
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "Item.h"
#include "Manager.h"

//QT includes
#include <QDebug>
#include <QGraphicsPixmapItem>
#include <QMessageBox>

//C++ includes
#include <thread>
#include <iostream>
#include <functional>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    /* Disconnect logic of program from GUI */
    std::unique_ptr<Manager> manager(new Manager(this));

    /* Quote from qtcentre forum: "If you are using locally allocated memory (eg. on the stack), then you don't want to delete that memory until the slot is called and completed, thus you would use a BlockingQueuedConnection." Blocking waits for the slot to complete before returning to execution in the thread emitting the signal. */
    QObject::connect(manager.get(), &Manager::Show_Icon, this, &MainWindow::Show_Icon, Qt::BlockingQueuedConnection);

    std::thread manager_thread(&Manager::Start, std::move(manager));
    manager_thread.detach();
}

MainWindow::~MainWindow()
{
    delete ui;
}

/**
 * @brief Right now a helper function, just to see if icons are loaded successfully.
 * @param icon
 */
void MainWindow::Show_Icon(QPixmap &icon)
{
    ui->ItemIcon->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    graphic_scene = std::make_unique<QGraphicsScene>(ui->ItemIcon);
    ui->ItemIcon->setScene(graphic_scene.get());
    graphic_scene->addPixmap(icon);

    /* Wait before another icon loads. */
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

void MainWindow::on_Start_Stats_Button_clicked() //Put this to manager object
{
    /*
    // Getting an item
    Item item, item_2;

    item.Set_Icon("/home/patryk/QT Projects/Manic LTime/Ellie.jpg");
    item.name = "Mozilla Firefox";
    item.time_statistics.total_hours = 2;
    item.time_statistics.total_minutes = 37;
    item.time_statistics.total_seconds = 15;


    item_2.Set_Icon("/home/patryk/QT Projects/Manic LTime/HeavyRain.png");
    item_2.name = "Eclipse IDE";
    item_2.time_statistics.total_hours = 45;
    item_2.time_statistics.total_minutes = 12;
    item_2.time_statistics.total_seconds = 53;

    Manager manager;
    manager.Add_Item(item);
    manager.Add_Item(item_2);




    ui->ItemIcon->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    graphic_scene = new QGraphicsScene(ui->ItemIcon);
    ui->ItemIcon->setScene(graphic_scene);



    QGraphicsPixmapItem *pixmapItem1 = graphic_scene->addPixmap(*item.icon);
    QGraphicsPixmapItem *pixmapItem2 = graphic_scene->addPixmap(*item_2.icon);

    pixmapItem2->moveBy(0, item.icon->height() + 5);






    //PACKING STRINGS
    QString object_names;

    for(map<QString, Item>::iterator it = manager.objects.begin(); it != manager.objects.end(); ++it)
    {
        object_names.append(it->first); object_names.append("\n");
    }

    QGraphicsTextItem *text = new QGraphicsTextItem;
    text->setPos(100, 0); //width, height
    text->setPlainText(item.name);
    graphic_scene->addItem(text);

    QGraphicsTextItem *text_2 = new QGraphicsTextItem;
    text_2->setPos(100, item.icon->height() + 1); //Get previous icon size, so current icon can have name next to it.
    text_2->setPlainText(item_2.name);
    graphic_scene->addItem(text_2);
    */
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QMessageBox::StandardButton question_exit = QMessageBox::question(this, "Do we say goodbye?", tr("Are you sure?"),
                                                                      QMessageBox::Yes | QMessageBox::No);
    if(question_exit == QMessageBox::Yes)
    {
        //TO DO: Save time stats, tell other thread to end his work.
        event->accept();
    }
    else
        event->ignore();
}




















