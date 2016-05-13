#ifndef ITEM_H
#define ITEM_H

#include <QPixmap>
#include <memory>

class Manager;

class Item /* This is a single application, that we observe. */
{
    friend class Manager;

private:
    QString name;
    std::shared_ptr<QPixmap> icon;

private:
    void Set_Icon(QString path);


public:
    Item(const Item &rhs);
    Item();

public:
    struct Time_Statistics
    {
        int total_hours = 0, total_minutes = 0, total_seconds = 0; //Time that application was run since beggining of watching.

        bool operator < (const Time_Statistics &rhs) const
        {
            return total_hours < rhs.total_hours;
        }
    } time_statistics;

signals:

public slots:
};
#endif // ITEM_H
