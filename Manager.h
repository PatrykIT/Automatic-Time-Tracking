#ifndef MANAGER_H
#define MANAGER_H

#include "Item.h"

#include <mainwindow.h>
#include <QObject>

#include <map>
#include <set>



class Manager : public QObject
{
    Q_OBJECT
private:
    std::string System_Call(const std::string &command);
    std::set<int> Get_PIDs_from_strings(std::vector<std::string> &input);
    std::vector<std::string> Split_Output_to_Strings(const std::string &input);
    static std::map<Item::Time_Statistics, Item> objects;

private:
    static void Add_Item(const Item &item);

public:
    explicit Manager(Ui::MainWindow &ui, QObject *parent = 0);

    Manager(const Manager &other);
    ~Manager();

    void Start();

public:
    Ui::MainWindow *ui_m;

signals:

public slots:
};

#endif // MANAGER_H
