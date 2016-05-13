#include "Item.h"
#include <QPixmap>

#include <memory>
Item::Item()
{

}

Item::Item(const Item &rhs) : name(rhs.name), time_statistics(rhs.time_statistics)
{
    icon = std::shared_ptr<QPixmap>(new QPixmap); //or : icon.reset(new QPixmap); Shared pointers are explicit. You can't just assign the ptr type to them.
    *icon = *rhs.icon;
}

void Item::Set_Icon(QString path)
{
    //icon = new QPixmap(path); //You can't assign a native pointer to a shared pointer. The shared_ptr must be initialized with that value, or you can call reset() with the native pointer value.
    icon = std::make_shared<QPixmap>(path);
}
