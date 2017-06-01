#ifndef MANAGER_H
#define MANAGER_H

//Local includes
#include <mainwindow.h>

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



class Manager_Interface
{
protected:
    virtual void Add_Item_to_Observe(const Item &item, Process_Statistics time_stats) = 0;
    virtual void Add_Item_to_Observe(Item &&item, Process_Statistics &&time_stats) = 0;

    virtual void Check_if_Applications_are_Running() = 0;
    virtual void Add_New_Observed_Objects() = 0;
    virtual std::vector<std::string> Get_Running_Applications() = 0;

    virtual void Save_Statistics_to_File() = 0;
    virtual void Load_Statistics_from_File() = 0;
    virtual std::tuple<std::string, int, int, int> Parse_File_Statistics(const std::string &line) const = 0;

    Manager_Interface() { }

public:
    virtual void Start() = 0;

    virtual ~Manager_Interface() = 0;
};


class Abstract_Manager : public QObject, protected Manager_Interface
{
    Q_OBJECT

/* Methods */
protected:
    //virtual void LOGS(const Logger &info) const = 0;

     /* TODO: Implement default behaviour */
    virtual void Add_Item_to_Observe(const Item &item, Process_Statistics time_stats);
    virtual void Add_Item_to_Observe(Item &&item, Process_Statistics &&time_stats);

    virtual void Check_if_Applications_are_Running();
    virtual void Add_New_Observed_Objects();

    virtual void Save_Statistics_to_File();
    virtual void Load_Statistics_from_File();
    virtual std::tuple<std::string, int, int, int> Parse_File_Statistics(const std::string &line) const;

/* Members */
protected:
    Process_Statistics proccess_statistic_object;
    std::vector<std::pair<Item, Process_Statistics>> applications;
    std::vector<std::string> processes_names;

    std::fstream file_stats;
    std::string path_to_stats_file;

    Abstract_Manager() { }


public:
    virtual void Print_Elapsed_Time() const;

signals:
    void Show_Icon(QPixmap icon);
};

class Linux_Manager : private Abstract_Manager
{
    Q_OBJECT

private:
    std::string System_Call(const std::string &command) const;
    std::set<int> Get_PIDs_from_Strings(std::vector<std::string> &input) const;
    std::vector<std::string> Split_Command_Output_to_Strings(const std::string &input) const;
    std::vector<std::string> Get_Processes_Names(const std::set<int> &pid_numbers) const;
    std::vector<std::string> Get_Running_Applications();
public:

};


class Logger
{
    friend class Abstract_Manager;
private:
    std::fstream log_file;
    std::string path_to_log_file = "Logs.txt";
    std::string TAG; //Each class that has a Logger saves its name as a TAG.

public:
    void Log_To_File(std::string log_message); //TODO: Add implementation
};






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

