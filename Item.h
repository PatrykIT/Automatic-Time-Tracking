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
    Item(const Item &rhs);
    Item();

public:
//    bool operator < (const Item &rhs) const
//    {
//        return true; //We do not care about order in std::map, so let's save on comparing strings.
//    }

    static int ID;

signals:

public slots:
};




#endif // ITEM_H
