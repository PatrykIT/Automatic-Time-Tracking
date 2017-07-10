#ifndef MANAGER_H
#define MANAGER_H

//Local includes
#include <mainwindow.h>
#include "Item.h"

//C++ includes
#include <set>
#include <chrono>
#include <fstream>
#include <tuple>

class Item;
struct Logger;


struct Process_Statistics
{
    Process_Statistics();
    ~Process_Statistics() = default;
    Process_Statistics(int hours, int minutes, int seconds);

    Process_Statistics(const Process_Statistics &statistics);
    Process_Statistics(Process_Statistics &&rhs) noexcept;
    Process_Statistics& operator = (const Process_Statistics &rhs)  = delete;

    void Stop_Counting_Time();
    void Parse_Time();
    int Parse_Minutes();
    int Parse_Seconds();

    bool operator < (const Process_Statistics &rhs) const noexcept
    {
        return total_hours < rhs.total_hours;
    }

    typedef std::chrono::high_resolution_clock _clock;
    std::chrono::time_point<_clock> begin_time;
    std::chrono::time_point<_clock> end_time;

    int total_hours, total_minutes, total_seconds; //Time that application was ran since beggining of watching.
    bool is_running;

    //typedef std::chrono::duration<double, std::ratio<1>> seconds; //It's more precise, but we don't need that much precision.
    typedef std::chrono::seconds seconds;
};






/* TODO: Consider PIMPL idiom or Template Method pattern.
 * https://isocpp.org/wiki/faq/strange-inheritance#private-virtuals
 * https://stackoverflow.com/questions/3970279/what-is-the-point-of-a-private-pure-virtual-function */

class Manager_Interface
{
protected:
    Manager_Interface() { }
    /* Herb's advice: A base class destructor should be either public and virtual, or protected and non-virtual. */
    ~Manager_Interface() = default; //Disallow polymorphic deletion through base pointer.

    /* TODO: Make this work as a private method. https://isocpp.org/wiki/faq/strange-inheritance#private-virtuals */
    virtual std::vector<std::string> Get_Running_Applications() = 0;
    virtual void Start() = 0;

private:
    virtual void Add_Item_to_Observe(const Item &item, Process_Statistics time_stats) = 0;
    virtual void Add_Item_to_Observe(Item &&item, Process_Statistics &&time_stats) = 0;

    virtual void Check_if_Applications_are_Running() = 0;
    virtual void Add_New_Observed_Objects() = 0;

    virtual void Save_Statistics_to_File() = 0;
    virtual void Load_Statistics_from_File() = 0;
    virtual std::tuple<std::string, int, int, int> Parse_File_Statistics(const std::string &line) const = 0;
};




class Abstract_OS_Manager : public QObject, protected Manager_Interface
{
    Q_OBJECT

/* Methods */
protected:
    explicit Abstract_OS_Manager(QObject *parent = 0);

    //virtual void LOGS(const Logger &info) const = 0;

private:
    /* TODO: Change this to public. Interface must be consistent with Linux / Windows manager.
    * User will use Abstract_OS_Manager *manager = new FactoryManager() // returns Linux or Windows manager
    * so those functions must be callable from outside of class (by the user).
    *
    * QUESTION: Should it be private or public? From inheritance, let's say in future someone would like to create iOS_Manager that inherits from this class.
    * Think about flexibility of private vs public in that case. */
    virtual void Add_Item_to_Observe(const Item &item, Process_Statistics time_stats) override;
    virtual void Add_Item_to_Observe(Item &&item, Process_Statistics &&time_stats) override;

    virtual void Check_if_Applications_are_Running() override;
    virtual void Add_New_Observed_Objects() override;

    virtual void Save_Statistics_to_File() override;
    virtual void Load_Statistics_from_File() override;
    virtual std::tuple<std::string, int, int, int> Parse_File_Statistics(const std::string &line) const override;



/* Members */
protected:
    Process_Statistics proccess_statistic_object;
    std::vector<std::pair<Item, Process_Statistics>> applications;
    std::vector<std::string> processes_names;

    std::fstream file_stats;
    std::string path_to_stats_file;

public:
    virtual ~Abstract_OS_Manager() = default;

    virtual void Start() override;
    virtual void Print_Elapsed_Time() const;



signals:
    void Show_Icon(QPixmap icon);
};




class Linux_Manager final : public Abstract_OS_Manager
{
    Q_OBJECT

private:
    std::string System_Call(const std::string &command) const;
    std::set<int> Get_PIDs_from_Strings(std::vector<std::string> &input) const;
    std::vector<std::string> Split_Command_Output_to_Strings(const std::string &input) const;
    std::vector<std::string> Get_Processes_Names(const std::set<int> &pid_numbers) const;
    std::vector<std::string> Get_Running_Applications() override;

public:
    explicit Linux_Manager(QObject *parent = 0);
    ~Linux_Manager() = default;

signals:
    void Show_Icon(QPixmap icon); //TODO: This signal should only be in Abstract_OS_Manager.
};







class Windows_Manager final : public Abstract_OS_Manager
{
    Q_OBJECT

private:
    std::vector<std::string> Get_Running_Applications() override;

public:
    explicit Windows_Manager(QObject *parent = 0);
    ~Windows_Manager() = default;

signals:
    void Show_Icon(QPixmap icon); //TODO: This signal should only be in Abstract_OS_Manager.
};










class Logger
{
    friend class Abstract_OS_Manager;
private:
    std::fstream log_file;
    std::string path_to_log_file = "Logs.txt";
    std::string TAG; //Each class that has a Logger saves its name as a TAG.

public:
    void Log_To_File(std::string log_message); //TODO: Add implementation
};






























/* -------------------------------------------------------------- OLD INTERFACE -------------------------------------------------------------- */

class Manager : public QObject
{
    Q_OBJECT

//Members
private:

    struct Process_Statistics
    {
        Process_Statistics();
        ~Process_Statistics() = default;
        Process_Statistics(int hours, int minutes, int seconds);

        Process_Statistics(const Process_Statistics &statistics);
        Process_Statistics(Process_Statistics &&rhs) noexcept;
        Process_Statistics& operator = (const Process_Statistics &rhs)  = delete;

        void Stop_Counting_Time();
        void Parse_Time();
        int Parse_Minutes();
        int Parse_Seconds();

        bool operator < (const Process_Statistics &rhs) const noexcept
        {
            return total_hours < rhs.total_hours;
        }

        typedef std::chrono::high_resolution_clock _clock;
        std::chrono::time_point<_clock> begin_time;
        std::chrono::time_point<_clock> end_time;

        int total_hours, total_minutes, total_seconds; //Time that application was ran since beggining of watching.
        bool is_running;

        //typedef std::chrono::duration<double, std::ratio<1>> seconds; //It's more precise, but we don't need that much precision.
        typedef std::chrono::seconds seconds;
    };

    static std::vector<std::pair<Item, Process_Statistics>> applications;

    std::fstream file_stats;
    const std::string path_to_stats_file = "Statistics.txt";


//Methods
private:
    void Add_Item_to_Observe(const Item &item, Process_Statistics time_stats);
    void Add_Item_to_Observe(Item &&item, Process_Statistics &&time_stats);

    void Save_Statistics_to_File();
    void Load_Statistics_from_File();
    void LOGS(const std::string& info) const;

    void Check_if_Applications_are_Running(std::vector<std::string> &processes_names);
    void Add_New_Observed_Objects(std::vector<std::string> &processes_names);

    std::tuple<std::string, int, int, int> Parse_File_Statistics(const std::string &line) const;

    /* --- Linux only --- */
    std::string System_Call(const std::string &command) const;
    std::set<int> Get_PIDs_from_Strings(std::vector<std::string> &input) const;
    std::vector<std::string> Split_Command_Output_to_Strings(const std::string &input) const;
    std::vector<std::string> Get_Processes_Names(const std::set<int> &pid_numbers) const;
    /* ----------------- */

    std::vector<std::string> Get_Running_Applications();


//Members
public:


//Methods
public:
    explicit Manager(QObject *parent = 0);
    
    Manager(const Manager &other) = delete;
    Manager(Manager &&other) = delete;
    
    Manager& operator = (const Manager &other) = delete;
    Manager& operator = (Manager &&other) = delete;
    
    ~Manager() = default;

    void Start();
    void Print_Elapsed_Time() const;



signals:
    void Show_Icon(QPixmap icon);

public slots:
};

#endif // MANAGER_H

