#ifndef ITEM_H
#define ITEM_H

//C++ includes
#include <QPixmap>
#include <memory>



class Item /* This is a single application, that we observe. */
{
    friend class Manager;
    friend class Abstract_Manager;

//Members
private:
    std::string name;
    std::unique_ptr<QPixmap> icon;

//Methods
private:
    void Load_Icon();
    void Scale_Icon(QPixmap &pixmap);

//Members
public:

//Methods
public:
    Item() = delete;
    explicit Item(const std::string &_name);
    explicit Item(std::string &&_name);
    ~Item() = default;

    Item(Item &&rhs) noexcept;
    Item& operator =(Item &&rhs) = delete;

    Item(const Item &rhs);
    Item& operator =(const Item &rhs) = delete;

signals:

public slots:
};




#endif // ITEM_H
