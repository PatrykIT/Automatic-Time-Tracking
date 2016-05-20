#ifndef MANAGER_H
#define MANAGER_H

#include <mainwindow.h>

#include <set>
#include <chrono>

class Item;
class Manager : public QObject
{
    Q_OBJECT

//Members
private:
    struct Process_Statistics
    {
        Process_Statistics();
        ~Process_Statistics();
        Process_Statistics(const Process_Statistics &statistics);
        double total_hours, total_minutes, total_seconds; //Time that application was run since beggining of watching.

        bool operator < (const Process_Statistics &rhs) const
        {
            return total_hours < rhs.total_hours;
        }

        typedef std::chrono::high_resolution_clock _clock;
        std::chrono::time_point<_clock> begin_time;
        std::chrono::time_point<_clock> end_time;
        double time_difference;

        //typedef std::chrono::duration<double, std::ratio<1>> seconds; //It's more precise, but we don't need that much precision.
        typedef std::chrono::seconds seconds;
        typedef std::chrono::minutes minutes;
    };

    static std::vector<std::pair<Item, Process_Statistics>> objects;


//Methods
private:
    static void Add_Item(const Item &item);

    std::string System_Call(const std::string &command);
    std::set<int> Get_PIDs_from_Strings(std::vector<std::string> &input);
    std::vector<std::string> Split_Command_Output_to_Strings(const std::string &input);
    std::vector<std::string> Get_Processes_Names(const std::set<int> &pid_numbers);


//Methods
public:
    explicit Manager(QObject *parent = 0);
    Manager(const Manager &other) = delete;
    ~Manager();

    void Start();
    void Print_Elapsed_Time();


//Members
public:

signals:

public slots:
};

#endif // MANAGER_H

/* Start_Counting_Time() will only set variables that tell at what time was the function started.
 * The Stop_Counting_Time() will retrieve this values, get new, and calculate difference.
 * Difference will be saved in Time Statistics.*/
