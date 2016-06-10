//Local includes
#include "Item.h"


Item::Item(const std::string &_name) : name(_name), icon(std::unique_ptr<QPixmap>(nullptr))
{
    //icon = std::unique_ptr<QPixmap> (new QPixmap("/home/patryk/QT Projects/Manic LTime/HeavyRain.png"));
}

Item::Item(std::string &&name) : name(std::move(name)), icon(std::unique_ptr<QPixmap>(nullptr))
{
    //icon = std::unique_ptr<QPixmap> (new QPixmap("/home/patryk/QT Projects/Manic LTime/HeavyRain.png"));
}

Item::Item(const Item &rhs) : name(rhs.name)
{
    if(rhs.icon != nullptr)
    {
        icon = std::unique_ptr<QPixmap>(new QPixmap);
        *icon = *rhs.icon;
    }
}

Item::Item(Item &&rhs)
{
    name = std::move(rhs.name);
    icon = std::move(icon);
}
