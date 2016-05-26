#ifndef ITEM_H
#define ITEM_H

#include <QPixmap>
#include <memory>

class Item /* This is a single application, that we observe. */
{
    friend class Manager;

private:
    std::string name;
    std::shared_ptr<QPixmap> icon;

private:
    void Set_Icon(QString path);

public:
    Item();
    explicit Item(const std::string &name);
    explicit Item(std::string &&name);
    Item(const Item &rhs);

public:

signals:

public slots:
};




#endif // ITEM_H
