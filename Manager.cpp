//Local includes
#include "Manager.h"
#include "ui_mainwindow.h"
#include "Item.h"

//QT includes
#include <QDebug>

//C++ includes
#include <iostream>
#include <thread> //sleep_for
#include <sstream>
#include <mutex>
#include <ctime>
#include <functional>

using std::cout;
using std::endl;

std::vector<std::pair<Item, Manager::Process_Statistics>> Manager::applications;

Manager::Manager(QObject *parent) : QObject(parent)
{
    applications.reserve(128);
}

Manager::Process_Statistics::Process_Statistics() : Process_Statistics(0, 0, 0)
{ }

Manager::Process_Statistics::Process_Statistics(int hours, int minutes, int seconds)
    : begin_time(Process_Statistics::_clock::now()), end_time((Process_Statistics::_clock::now())),
      total_hours(hours), total_minutes(minutes), total_seconds(seconds),
      is_running(true)
{ }

Manager::Process_Statistics::Process_Statistics(const Process_Statistics &statistics)
    : begin_time(statistics.begin_time), end_time(statistics.end_time),
      total_hours (statistics.total_hours), total_minutes(statistics.total_minutes), total_seconds(statistics.total_seconds),
      is_running(statistics.is_running)
{ }

Manager::Process_Statistics::Process_Statistics(Process_Statistics &&rhs) noexcept
    : begin_time(std::move(rhs.begin_time)), end_time(std::move(rhs.end_time)),
    total_hours(rhs.total_hours), total_minutes(rhs.total_minutes), total_seconds(rhs.total_seconds),
    is_running(rhs.is_running)
{ }


/**
 * @brief Main observing loop.
 */
void Manager::Start()
{
    /* We're calling Start() from MainWindow constructor. Let's delay start so GUI is all ready. */
    /* TO DO: Change it to bool (shared bool visible for read-only access by Manager). With this bool do pooling, so while(false) { poll } */
    std::this_thread::sleep_for(std::chrono::seconds(1));

    try
    {
        /* Add processes from file */
        Load_Statistics_from_File();

        std::vector<std::string> processes_names;
        processes_names.reserve(128);

        int counter = 0;
        /* Observe if there are new processes, and if old ones are still ON. */
        while(counter < 5)
        {
            processes_names = Get_Running_Applications();

            if(!processes_names.empty())
            {
                /* Two-way check.
                 * 1: Check if items (names) in vector<applications> are now in vector <processes names>. If not, stop counting time for them - they were switched off.
                 * 2: Check if apps that we are observing now (vector<processes_names>) are in our observer (vector<applications>). If not, add them to observer.
                 * */

                /* 1-way check */
                Check_if_Applications_are_Running(processes_names);
                /* 2-way check */
                Add_New_Observed_Objects(processes_names);

                //LOGS("\n");
            }
            ++counter;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        Save_Statistics_to_File();
        cout << "My work is done." << endl;
    }
    catch(std::ios_base::failure &exception)
    {
        qDebug() << "Exception caught: " << exception.what();
        //TO DO: Save Time. Input/Output failed, so it would be better to try to save time online, in a cloud.
    }
    catch(std::runtime_error &exception)
    {
        qDebug() << "Exception caught: " << exception.what();
        LOGS(std::string("ERROR! runtime_error exception caught: ") + exception.what());
        //TO DO: Save Time
    }
    catch(std::bad_alloc &exception)
    {
        qDebug() << "Exception caught: " << exception.what();
        LOGS(std::string("ERROR! runtime_error exception caught: ") + exception.what());
        //TO DO: Save Time
    }
    catch (...)
    {
        qDebug() << "Unknow exception caught.";
        LOGS("ERROR! Unknow exception.");
        //TO DO: Save Time
    }
}

/**
 * @brief Executes shell command, parses its output to one line strings, converts PIDs from string to int,
 * translates those PIDs to processes names.
 * @return Names of processes currently running on PC.
 */
std::vector<std::string> Manager::Get_Running_Applications()
{
#ifdef __linux__
    std::string command = "wmctrl -lp | grep -o '0x[0-9a-z]*\s*  [0-9] [0-9]\\{1,8\\}'";
    std::string system_call_result = System_Call(command);
    //LOGS("wmctrl output: " + system_call_result);

    if(system_call_result.empty())
    {
        //LOGS("ERROR! Wmctrl output is empty.");
        /* Return an empty vector. */
        return std::vector<std::string>();
    }

    std::vector<std::string> strings_split = Split_Command_Output_to_Strings(system_call_result);
    std::set<int> pid_numbers = Get_PIDs_from_Strings(strings_split);

    for(std::set<int>::iterator it = pid_numbers.begin(); it != pid_numbers.end(); ++it)
    {
        /* Sometimes wmctrl will give process ID 0, which is an error. So we are getting rid of it. */
        if(*it == 0)
        {
            pid_numbers.erase(it);
        }
    }

    return Get_Processes_Names(pid_numbers);

#else
    return {};
#endif
}

/**
 * @brief Executes shell comand.
 * @param command - command name to be called
 * @return whole output of command call
 */
std::string Manager::System_Call(const std::string &command) const
{
    const int buffer_size = 1024;
    char buffer[buffer_size];
    std::string result = "";

    #ifdef __linux__

    /* Use unique_ptr with custom deleter. */
    std::unique_ptr<FILE, decltype(&pclose)> pipe (popen(command.c_str(), "r"), pclose);

    if (!pipe)
        throw std::runtime_error("popen() failed!");

    /* Append output from system call to string. */
    while (feof(pipe.get()) == 0)
    {
        if (fgets(buffer, buffer_size, pipe.get()) != NULL) //You can happily use fgets() to read from a file stream created by popen()
        {
            result += buffer;
        }
    }
    #endif

    return result;
}

/**
 * @brief Processes input that looks like '0x03000001  0 1883' and gets out of it last serie of digits - which is PID number of a process.
 * @param input - vector of strings, in which each string is a separate line looking like '0x03000001  0 1883'.
 * @return set of PIDs
 */
std::set<int> Manager::Get_PIDs_from_Strings(std::vector<std::string> &input) const
{
    std::set<int> pid_numbers;
    std::string pid = "";

    for(std::vector<std::string>::iterator it = input.begin(); it != input.end(); ++it)
    {
        /* First serie of characters from the end is a pid number. Save it, and repeat loop, until we get all PIDs. */
        for(std::string::reverse_iterator string_iterator = (*it).rbegin(); string_iterator != (*it).rend(); ++string_iterator)
        {
            if(isdigit(*string_iterator))
            {
                pid += *string_iterator;
            }
            else
            {
                /* We finished copying number to string, now we have to reverse it back to original PID. */
                std::reverse(pid.begin(), pid.end());
                pid_numbers.insert(std::stoi(pid));
                pid.clear();

                break;
            }
        }
    }

    return pid_numbers;
}

/**
 * @brief Split_Output_to_Strings
 * @param input - result of calling system call
 * @return vector of split output - one string as one line - looking like '0x02000002  0 1315'
 */
std::vector<std::string> Manager::Split_Command_Output_to_Strings(const std::string &input) const
{
    /* Each line is contained as separate string. */
    std::vector <std::string> output_line;
    std::string current_line = "";

    /* Traverse whole output, to split strings line by line. */
    for(std::string::const_iterator output = input.begin(); output != input.end(); ++output)
    {
        if(*output != '\n')
            current_line += *output;
        else
        {
            output_line.push_back(current_line);
            current_line.clear();
        }
    }

    return output_line;
}

/**
 * @brief Calls 'ps' syscall to get processes names based on their PIDs.
 * @param pid_numbers - PIDs to be translated to names of processes.
 * @return vector of processes names.
 */

std::vector<std::string> Manager::Get_Processes_Names(const std::set<int> &pid_numbers) const
{
    std::vector<std::string> processes_names;
    //std::string log_string = "";

    /* Call 'ps' syscall to get processes names based on their PIDs */
    for(auto PID_nr = pid_numbers.begin(); PID_nr != pid_numbers.end(); ++PID_nr)
    {
        /* Get process name based on its PID */
        std::string process_name = System_Call("ps -p " + std::to_string(*PID_nr) + " -o comm=");

        /* If output is bad, ignore it. */
        if(process_name.empty() || process_name.back() != '\n')
        {
            //log_string.append(" " + std::string("ERROR! Process doesn't have a newline at the end or is empty. Pid number: ") + std::to_string(*it));
            continue;
        }
        /* Remove new line from the end of string. */
        process_name.pop_back();

        //log_string.append(" " + process_name);

        processes_names.emplace_back(std::move(process_name));
    }

    //LOGS("ps output: " + log_string);
    return processes_names;
}

void Manager::Add_Item_to_Observe(const Item &item, Process_Statistics time_stats)
{
    applications.emplace_back(std::pair<Item, Manager::Process_Statistics> (item, time_stats));
}

void Manager::Add_Item_to_Observe(Item &&item, Process_Statistics &&time_stats)
{
    applications.emplace_back(std::pair<Item, Manager::Process_Statistics> (std::move(item), std::move(time_stats)));
}

void Manager::Print_Elapsed_Time() const
{
    std::for_each(applications.begin(), applications.end(), [](const std::pair<Item, Process_Statistics> &object)
    {
        cout << "Time of: " << object.first.name <<" is: " << object.second.total_hours << ":"
             << object.second.total_minutes << ":" << object.second.total_seconds << endl;
    });
}


/**
 * @brief Checks whether observed applications went OFF, or if they were restarted (ON -> OFF -> ON).
 * @param processes_names - vector of processes names currently running on PC.
 */
void Manager::Check_if_Applications_are_Running(std::vector<std::string> &processes_names)
{
    for(std::pair<Item, Process_Statistics> &item : applications)
    {
        /* If application was being observed (or was saved in statistics file), but now it is OFF. */
        if(std::find_if(processes_names.begin(), processes_names.end(), [&item](const std::string &name)
        { return name == item.first.name; }) == processes_names.end())
        {
            if(item.second.is_running)
            {
                cout << item.first.name << " was being observed, but now it is off." << endl << endl;
                item.second.Stop_Counting_Time();
                item.second.Parse_Time();
                item.second.is_running = false;
            }
        }
        else
        {
            /* If applicaiton was being observed, then switched OFF, and then started again, we must continue with counting time for it. */
            if(item.second.is_running == false)
            {
                cout << "Application: " << item.first.name << " was restarted." << endl;
                item.second.is_running = true;
                /* Start counting time again. */
                item.second.begin_time = Process_Statistics::_clock::now();
            }
        }
    }
}


/**
 * @brief Checks if there are new processes running. If so, then adds them for being observed.
 * @param processes_names - vector of processes names currently running on PC.
 */
void Manager::Add_New_Observed_Objects(std::vector<std::string> &processes_names)
{
    for(auto &name : processes_names)
    {
        /* Check if process currently running is in our observer vector 'applications'. If not, add it for counting time. */
        if(std::find_if(applications.begin(), applications.end(), [&name](const std::pair<Item, Manager::Process_Statistics> &rhs)
        { return name == rhs.first.name; }) == applications.end())
        {
            /* Add new process */
            Item item(std::move(name));
            cout << "Adding new item: " << item.name << endl << endl;
            Add_Item_to_Observe(std::move(item), Process_Statistics());
        }
    }
}

/**
 * @brief Counts time for all objects that were or still are observed, and saves to a file.
 */
void Manager::Save_Statistics_to_File()
{
    file_stats.open(path_to_stats_file, std::fstream::out);
    if(!file_stats.is_open())
        throw std::ios_base::failure("Couldn't open a file in order to save statistics: " + path_to_stats_file);

    for(auto &object : applications)
    {
        if(object.second.is_running)
            object.second.Stop_Counting_Time();

        object.second.Parse_Time();
        file_stats << object.first.name << " ::: " << object.second.total_hours << ":" << object.second.total_minutes << ":"
                   << object.second.total_seconds << "\n";
    }
    file_stats.close();
}

/**
 * @brief Loads name of processess with theirs time, stores those information in tuple, creates Item and Process_Statistics objects based on those informations and calls Add_Item().
 */
void Manager::Load_Statistics_from_File()
{
    file_stats.open(path_to_stats_file, std::fstream::in);
    if(!file_stats.is_open())
        return; //File doesn't exists. Either it was first time the user run an app, and there is nothing to load from, or it was deleted.

    std::string line;
    while(std::getline(file_stats, line))
    {
        if(line.size() > 0)
        {
            /* <Name of process, hours, minutes, seconds> */
            std::tuple<std::string, int, int, int> process_information (Parse_File_Statistics(line));

            Item item(std::move (std::get<0>(process_information) ));
            Process_Statistics statistics(std::get<1>(process_information), std::get<2>(process_information), std::get<3>(process_information));


            if(item.icon != nullptr)
                emit Show_Icon(*item.icon);

            Add_Item_to_Observe(std::move(item), std::move(statistics));
        }
        else
            qDebug() << "Line empty.";
    }
    file_stats.close();
}


void Manager::Process_Statistics::Parse_Time()
{
    total_seconds = Parse_Seconds();
    total_minutes = Parse_Minutes();
}


void Manager::Process_Statistics::Stop_Counting_Time()
{
    end_time = Process_Statistics::_clock::now();
}

/**
 * @brief Counts how long application was ON, in seconds. If seconds > 59, then increments minutes until seconds are in range [0 - 59]
 * @return Seconds in range [0 - 59] spent in application.
 */
int Manager::Process_Statistics::Parse_Seconds()
{
    int seconds = std::chrono::duration_cast<Process_Statistics::seconds>(end_time - begin_time).count();
    seconds += total_seconds;

    while(seconds > 59)
    {
        ++total_minutes;
        seconds = seconds - 60;
    }

    return seconds;
}

/**
 * @brief Minutes are set by Parse_Seconds(). If minutes > 59, function increments hours until minutes are in range [0 - 59]
 * @return Minutes in range [0 - 59] spent in application.
 */
int Manager::Process_Statistics::Parse_Minutes()
{
    while(total_minutes > 59)
    {
        ++total_hours;
        total_minutes = total_minutes - 60;
    }

    return total_minutes;
}


/**
 * @brief We want to parse the string (one line from file) to get name and time (separately hour, minute, seconde).
 * @param line - looks like 'chrome ::: 0:24:35'
 * @return a tuple, that consists of name, hours, minutes, seconds.
 */
std::tuple<std::string, int, int, int> Manager::Parse_File_Statistics(const std::string &line) const
{
   std::stringstream line_stream (line);
   std::string name_of_process;
   /* Save first word from line, which is always a process name. */
   line_stream >> name_of_process;

    std::vector<int> time;
    time.reserve(3);
    int hours, minutes, secondes;

    /* Finds position in string where time starts. */
    for(std::string::const_iterator string_iterator = line.begin(); string_iterator != line.end(); ++string_iterator)
    {
        /* Check for ':::' After this (and whitespace) will be time */
        if(*string_iterator == ':' && *(string_iterator +1) == ':' && *(string_iterator +2) == ':')
        {
            /* string_iterator + 3 would point to a whitespace, so skip it */
            for(std::string::const_iterator time_iterator =  string_iterator + 4; time_iterator != line.end(); ++time_iterator)
            {
                /* If unit of time is a single digit. */
                if (*(time_iterator + 1) == ':'  || (time_iterator + 1) == line.end())
                {
                    /* Convert char to number */
                    time.push_back(static_cast<int> (*time_iterator) - 48);
                    time_iterator++; //This makes time iterator point to semicolon. When loop makes another iteration it will land after semicolon, which is what we want.
                    if(time_iterator == line.end())
                     break;
                }
                /* If unit of time is a double digit. */
                else if(*(time_iterator + 2) == ':' || (time_iterator + 2) == line.end())
                {
                    std::string double_time_value = "";
                    double_time_value += *time_iterator; /* First digit */
                    double_time_value += *(time_iterator + 1); /* Second digit */
                    /* Convert 2digit string to number */
                    time.push_back(std::stoi(double_time_value));

                    time_iterator += 2;
                    if(time_iterator == line.end())
                        break;
                }
            }
            hours = time.at(0);
            minutes = time.at(1);
            secondes = time.at(2);

            return std::make_tuple(name_of_process, hours, minutes, secondes);
        }
    }
    throw std::ios_base::failure("Couldn't find matching pattern - :::");
}


/**
 * @brief Useful debugging info. The first time it is called it will append date to a file.
 */
std::once_flag Debug_Date_Once;

void Manager::LOGS(const std::string &info) const
{
    static std::fstream log_file;
    static std::string path_to_log_file = "Logs.txt";

    log_file.open(path_to_log_file, std::fstream::out | std::fstream::app);

    std::call_once(Debug_Date_Once, [this] ()
    {
        /* Get current time */
        time_t t = time(0);
        localtime(&t);

        /* Cast time to string */
        char *date = ctime(&t);
        log_file << "\t\t\t\t\t" << date;
    } );

    log_file << info << endl;
    log_file.close();
}








Process_Statistics::Process_Statistics() : Process_Statistics(0, 0, 0)
{ }


Process_Statistics::Process_Statistics(int hours, int minutes, int seconds)
    : begin_time(Process_Statistics::_clock::now()), end_time((Process_Statistics::_clock::now())),
      total_hours(hours), total_minutes(minutes), total_seconds(seconds),
      is_running(true)
{ }

Process_Statistics::Process_Statistics(const Process_Statistics &statistics)
    : begin_time(statistics.begin_time), end_time(statistics.end_time),
      total_hours (statistics.total_hours), total_minutes(statistics.total_minutes), total_seconds(statistics.total_seconds),
      is_running(statistics.is_running)
{ }


Process_Statistics::Process_Statistics(Process_Statistics &&rhs) noexcept
    : begin_time(std::move(rhs.begin_time)), end_time(std::move(rhs.end_time)),
    total_hours(rhs.total_hours), total_minutes(rhs.total_minutes), total_seconds(rhs.total_seconds),
    is_running(rhs.is_running)
{ }


void Process_Statistics::Parse_Time()
{
    total_seconds = Parse_Seconds();
    total_minutes = Parse_Minutes();
}


void Process_Statistics::Stop_Counting_Time()
{
    end_time = Process_Statistics::_clock::now();
}


int Process_Statistics::Parse_Seconds()
{
    int seconds = std::chrono::duration_cast<Process_Statistics::seconds>(end_time - begin_time).count();
    seconds += total_seconds;

    while(seconds > 59)
    {
        ++total_minutes;
        seconds = seconds - 60;
    }

    return seconds;
}


int Process_Statistics::Parse_Minutes()
{
    while(total_minutes > 59)
    {
        ++total_hours;
        total_minutes = total_minutes - 60;
    }

    return total_minutes;
}





















Abstract_OS_Manager::Abstract_OS_Manager(QObject *parent)
{
    applications.reserve(128);
    processes_names.reserve(128);
}

/* TODO: Change this to universal reference and std::forward objects. */
void Abstract_OS_Manager::Add_Item_to_Observe(const Item &item, Process_Statistics time_stats)
{
    applications.emplace_back(std::pair<Item, Process_Statistics> (item, time_stats));
}


void Abstract_OS_Manager::Add_Item_to_Observe(Item &&item, Process_Statistics &&time_stats)
{
    applications.emplace_back(std::pair<Item, Process_Statistics> (std::move(item), std::move(time_stats)));
}


void Abstract_OS_Manager::Check_if_Applications_are_Running()
{
    for(std::pair<Item, Process_Statistics> &item : applications)
    {
        /* If application was being observed (or was saved in statistics file), but now it is OFF. */
        if(std::find_if(processes_names.begin(), processes_names.end(), [&item](const std::string &name)
        { return name == item.first.name; }) == processes_names.end())
        {
            if(item.second.is_running)
            {
                cout << item.first.name << " was being observed, but now it is off." << endl << endl;
                item.second.Stop_Counting_Time();
                item.second.Parse_Time();
                item.second.is_running = false;
            }
        }
        else
        {
            /* If applicaiton was being observed, then switched OFF, and then started again, we must continue with counting time for it. */
            if(item.second.is_running == false)
            {
                cout << "Application: " << item.first.name << " was restarted." << endl;
                item.second.is_running = true;
                /* Start counting time again. */
                item.second.begin_time = Process_Statistics::_clock::now();
                //item.second.begin_time = proccess_statistic_object._clock::now(); //Error message: Process_Statistics::_clock is not a base of 'Process_Statistics'
            }
        }
    }
}

void Abstract_OS_Manager::Add_New_Observed_Objects()
{
    for(auto &name : processes_names)
    {
        /* Check if process currently running is in our observer vector 'applications'. If not, add it for counting time. */
        if(std::find_if(applications.begin(), applications.end(), [&name](const std::pair<Item, Process_Statistics> &rhs)
        { return name == rhs.first.name; }) == applications.end())
        {
            /* Add new process */
            Item item(std::move(name));
            cout << "Adding new item: " << item.name << endl << endl;
            Add_Item_to_Observe(std::move(item), Process_Statistics());
        }
    }
}

void Abstract_OS_Manager::Save_Statistics_to_File()
{
    file_stats.open(path_to_stats_file, std::fstream::out);
    if(!file_stats.is_open())
        throw std::ios_base::failure("Couldn't open a file in order to save statistics: " + path_to_stats_file);

    for(auto &object : applications)
    {
        if(object.second.is_running)
            object.second.Stop_Counting_Time();

        object.second.Parse_Time();
        file_stats << object.first.name << " ::: " << object.second.total_hours << ":" << object.second.total_minutes
                   << ":" << object.second.total_seconds << "\n";
    }
    file_stats.close();
}

void Abstract_OS_Manager::Load_Statistics_from_File()
{
    file_stats.open(path_to_stats_file, std::fstream::in);
    if(!file_stats.is_open())
        return; //File doesn't exists. Either it was first time the user run an app, and there is nothing to load from, or it was deleted.

    std::string line;
    while(std::getline(file_stats, line))
    {
        if(line.size() > 0)
        {
            /* <Name of process, hours, minutes, seconds> */
            std::tuple<std::string, int, int, int> process_information (Parse_File_Statistics(line));

            Item item(std::move (std::get<0>(process_information) ));
            Process_Statistics statistics(std::get<1>(process_information), std::get<2>(process_information),
                                          std::get<3>(process_information));


            if(item.icon != nullptr)
                emit Show_Icon(*item.icon);

            Add_Item_to_Observe(std::move(item), std::move(statistics));
        }
        else
            qDebug() << "Line empty.";
    }
    file_stats.close();
}

std::tuple<std::string, int, int, int> Abstract_OS_Manager::Parse_File_Statistics(const std::string &line) const
{
    std::stringstream line_stream (line);
    std::string name_of_process;
    /* Save first word from line, which is always a process name. */
    line_stream >> name_of_process;

     std::vector<int> time;
     time.reserve(3);
     int hours, minutes, secondes;

     /* Finds position in string where time starts. */
     for(std::string::const_iterator string_iterator = line.begin(); string_iterator != line.end(); ++string_iterator)
     {
         /* Check for ':::' After this (and whitespace) will be time */
         if(*string_iterator == ':' && *(string_iterator +1) == ':' && *(string_iterator +2) == ':')
         {
             /* string_iterator + 3 would point to a whitespace, so skip it */
             for(std::string::const_iterator time_iterator =  string_iterator + 4; time_iterator != line.end();
                 ++time_iterator)
             {
                 /* If unit of time is a single digit. */
                 if (*(time_iterator + 1) == ':'  || (time_iterator + 1) == line.end())
                 {
                     /* Convert char to number */
                     time.push_back(static_cast<int> (*time_iterator) - 48);
                     time_iterator++; //This makes time iterator point to semicolon. When loop makes another iteration it will land after semicolon, which is what we want.
                     if(time_iterator == line.end())
                      break;
                 }
                 /* If unit of time is a double digit. */
                 else if(*(time_iterator + 2) == ':' || (time_iterator + 2) == line.end())
                 {
                     std::string double_time_value = "";
                     double_time_value += *time_iterator; /* First digit */
                     double_time_value += *(time_iterator + 1); /* Second digit */
                     /* Convert 2digit string to number */
                     time.push_back(std::stoi(double_time_value));

                     time_iterator += 2;
                     if(time_iterator == line.end())
                         break;
                 }
             }
             hours = time.at(0);
             minutes = time.at(1);
             secondes = time.at(2);

             return std::make_tuple(name_of_process, hours, minutes, secondes);
         }
     }
     throw std::ios_base::failure("Couldn't find matching pattern - :::");
}


void Abstract_OS_Manager::Print_Elapsed_Time() const
{
    std::for_each(applications.begin(), applications.end(), [](const std::pair<Item, Process_Statistics> &object)
    {
        cout << "Time of: " << object.first.name <<" is: " << object.second.total_hours << ":"
             << object.second.total_minutes << ":" << object.second.total_seconds << endl;
    });
}


void Abstract_OS_Manager::Start()
{
    /* We're calling Start() from MainWindow constructor. Let's delay start so GUI is all ready. */
    /* TO DO: Change it to bool (shared bool visible for read-only access by Manager). With this bool do pooling, so while(false) { poll } */
    std::this_thread::sleep_for(std::chrono::seconds(1));

    try
    {
        /* Add processes from file */
        Load_Statistics_from_File();


        int counter = 0;
        /* Observe if there are new processes, and if old ones are still ON. */
        while(counter < 5)
        {
            processes_names = Get_Running_Applications();

            if(!processes_names.empty())
            {
                /* Two-way check.
                 * 1: Check if items (names) in vector<applications> are now in vector <processes names>. If not, stop counting time for them - they were switched off.
                 * 2: Check if apps that we are observing now (vector<processes_names>) are in our observer (vector<applications>). If not, add them to observer.
                 * */

                /* 1-way check */
                Check_if_Applications_are_Running();
                /* 2-way check */
                Add_New_Observed_Objects();

                //LOGS("\n");
            }
            ++counter;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        Save_Statistics_to_File();
        cout << "My work is done." << endl;
    }
    catch(std::ios_base::failure &exception)
    {
        qDebug() << "Exception caught: " << exception.what();
        //TO DO: Save Time. Input/Output failed, so it would be better to try to save time online, in a cloud.
    }
    catch(std::runtime_error &exception)
    {
        qDebug() << "Exception caught: " << exception.what();
        //LOGS(std::string("ERROR! runtime_error exception caught: ") + exception.what());
        //TO DO: Save Time
    }
    catch(std::bad_alloc &exception)
    {
        qDebug() << "Exception caught: " << exception.what();
        //LOGS(std::string("ERROR! runtime_error exception caught: ") + exception.what());
        //TO DO: Save Time
    }
    catch (...)
    {
        qDebug() << "Unknow exception caught.";
        //LOGS("ERROR! Unknow exception.");
        //TO DO: Save Time
    }
}








Linux_Manager::Linux_Manager(QObject *parent) : Abstract_OS_Manager(parent)
{

}


/**
 * @brief Executes shell comand.
 * @param command - command name to be called
 * @return whole output of command call
 */
std::string Linux_Manager::System_Call(const std::string &command) const
{
    const int buffer_size = 1024;
    char buffer[buffer_size];
    std::string result = "";

    /* Use unique_ptr with custom deleter. */
    std::unique_ptr<FILE, decltype(&pclose)> pipe (popen(command.c_str(), "r"), pclose);

    if (!pipe)
        throw std::runtime_error("popen() failed!");

    /* Append output from system call to string. */
    while (feof(pipe.get()) == 0)
    {
        if (fgets(buffer, buffer_size, pipe.get()) != NULL) //You can happily use fgets() to read from a file stream created by popen()
        {
            result += buffer;
        }
    }

    return result;
}


/**
 * @brief Processes input that looks like '0x03000001  0 1883' and gets out of it last serie of digits - which is PID number of a process.
 * @param input - vector of strings, in which each string is a separate line looking like '0x03000001  0 1883'.
 * @return set of PIDs
 */
std::set<int> Linux_Manager::Get_PIDs_from_Strings(std::vector<std::string> &input) const
{
    std::set<int> pid_numbers;
    std::string pid = "";

    for(std::vector<std::string>::iterator it = input.begin(); it != input.end(); ++it)
    {
        /* First serie of characters from the end is a pid number. Save it, and repeat loop, until we get all PIDs. */
        for(std::string::reverse_iterator string_iterator = (*it).rbegin(); string_iterator != (*it).rend();
            ++string_iterator)
        {
            if(isdigit(*string_iterator))
            {
                pid += *string_iterator;
            }
            else
            {
                /* We finished copying number to string, now we have to reverse it back to original PID. */
                std::reverse(pid.begin(), pid.end());
                pid_numbers.insert(std::stoi(pid));
                pid.clear();

                break;
            }
        }
    }

    return pid_numbers;
}


/**
 * @brief Split_Output_to_Strings
 * @param input - result of calling system call
 * @return vector of split output - one string as one line - looking like '0x02000002  0 1315'
 */
std::vector<std::string> Linux_Manager::Split_Command_Output_to_Strings(const std::string &input) const
{
    /* Each line is contained as separate string. */
    std::vector <std::string> output_line;
    std::string current_line = "";

    /* Traverse whole output, to split strings line by line. */
    for(std::string::const_iterator output = input.begin(); output != input.end(); ++output)
    {
        if(*output != '\n')
            current_line += *output;
        else
        {
            output_line.push_back(current_line);
            current_line.clear();
        }
    }

    return output_line;
}


std::vector<std::string> Linux_Manager::Get_Processes_Names(const std::set<int> &pid_numbers) const
{
    std::vector<std::string> processes_names;
    //std::string log_string = "";

    /* Call 'ps' syscall to get processes names based on their PIDs */
    for(auto PID_nr = pid_numbers.begin(); PID_nr != pid_numbers.end(); ++PID_nr)
    {
        /* Get process name based on its PID */
        std::string process_name = System_Call("ps -p " + std::to_string(*PID_nr) + " -o comm=");

        /* If output is bad, ignore it. */
        if(process_name.empty() || process_name.back() != '\n')
        {
            //log_string.append(" " + std::string("ERROR! Process doesn't have a newline at the end or is empty. Pid number: ") + std::to_string(*it));
            continue;
        }
        /* Remove new line from the end of string. */
        process_name.pop_back();

        //log_string.append(" " + process_name);

        processes_names.emplace_back(std::move(process_name));
    }

    //LOGS("ps output: " + log_string);
    return processes_names;
}


/**
 * @brief Executes shell command, parses its output to one line strings, converts PIDs from string to int,
 * translates those PIDs to processes names.
 * @return Names of processes currently running on PC.
 */
std::vector<std::string> Linux_Manager::Get_Running_Applications()
{
    std::string command = "wmctrl -lp | grep -o '0x[0-9a-z]*\s*  [0-9] [0-9]\\{1,8\\}'";
    std::string system_call_result = System_Call(command);
    //LOGS("wmctrl output: " + system_call_result);

    if(system_call_result.empty())
    {
        //LOGS("ERROR! Wmctrl output is empty.");
        /* Return an empty vector. */
        return std::vector<std::string>();
    }

    std::vector<std::string> strings_split = Split_Command_Output_to_Strings(system_call_result);
    std::set<int> pid_numbers = Get_PIDs_from_Strings(strings_split);

    for(std::set<int>::iterator it = pid_numbers.begin(); it != pid_numbers.end(); ++it)
    {
        /* Sometimes wmctrl will give process ID 0, which is an error. So we are getting rid of it. */
        if(*it == 0)
        {
            pid_numbers.erase(it);
        }
    }

    return Get_Processes_Names(pid_numbers);
}




Windows_Manager::Windows_Manager(QObject *parent) : Abstract_OS_Manager(parent)
{

}

std::vector<std::string> Windows_Manager::Get_Running_Applications()
{

}





















