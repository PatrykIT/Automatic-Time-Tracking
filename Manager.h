#ifndef MANAGER_H
#define MANAGER_H

#include <mainwindow.h>

#include <set>
#include <chrono>
#include <iostream>
#include <fstream>

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
        int total_hours, total_minutes, total_seconds; //Time that application was run since beggining of watching.

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
        typedef std::chrono::minutes hours;

        void Stop_Counting_Time();
        void Parse_Time();
        int Parse_Minutes() const;
        int Parse_Seconds() const;
    };

    static std::vector<std::pair<Item, Process_Statistics>> objects;

    std::fstream file_stats;
    std::string path_to_file = "Statistics.txt";


//Methods
private:
    static void Add_Item(const Item &item);
    void Save_Statistics_to_File();
    void Load_Statistics_from_File();


    std::string System_Call(const std::string &command) const;
    std::set<int> Get_PIDs_from_Strings(std::vector<std::string> &input) const;
    std::vector<std::string> Split_Command_Output_to_Strings(const std::string &input) const;
    std::vector<std::string> Get_Processes_Names(const std::set<int> &pid_numbers) const;


//Methods
public:
    explicit Manager(QObject *parent = 0);
    Manager(const Manager &other) = delete;
    ~Manager();

    void Start();
    void Print_Elapsed_Time() const;


//Members
public:

signals:

public slots:
};

#endif // MANAGER_H

/* Start_Counting_Time() will only set variables that tell at what time was the function started.
 * The Stop_Counting_Time() will retrieve this values, get new, and calculate difference.
 * Difference will be saved in Time Statistics.*/
