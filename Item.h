#ifndef ITEM_H
#define ITEM_H

//C++ includes
#include <QPixmap>
#include <memory>

class Item /* This is a single application, that we observe. */
{
    friend class Manager;

private:
    std::string name;
    std::unique_ptr<QPixmap> icon;
    const std::string icons_path = "/usr/share/icons/hicolor/";

private:
    void Load_Icon();

public:
    Item() = delete;
    explicit Item(const std::string &_name);
    explicit Item(std::string &&name);
    Item(Item &&rhs) noexcept;
    Item(const Item &rhs);

public:

signals:

public slots:
};




#endif // ITEM_H
