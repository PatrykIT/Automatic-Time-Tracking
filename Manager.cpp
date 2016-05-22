//Local includes
#include "Manager.h"
#include "ui_mainwindow.h"
#include "Item.h"

//QT includes
#include <QDebug>

//C++ includes
#include <thread> //sleep_for
#include <iostream>

using std::cout;
using std::endl;

std::vector<std::pair<Item, Manager::Process_Statistics>> Manager::objects;

Manager::Manager(QObject *parent) : QObject(parent)
{
    objects.reserve(128);
}

Manager::Process_Statistics::Process_Statistics() : total_hours(0), total_minutes(0), total_seconds(0),
    begin_time(Process_Statistics::_clock::now())
{ }

Manager::Process_Statistics::~Process_Statistics()
{
    end_time = Process_Statistics::_clock::now();
    time_difference = std::chrono::duration_cast<Process_Statistics::seconds>(end_time - begin_time).count();
}

Manager::Process_Statistics::Process_Statistics(const Process_Statistics &statistics)
    : total_hours (statistics.total_hours), total_minutes(statistics.total_minutes), total_seconds(statistics.total_seconds),
      begin_time(statistics.begin_time), end_time(statistics.end_time), time_difference(statistics.time_difference)
{ }

Manager::~Manager()
{

}

/**
 * @brief System_Call
 * @param command - command name to be called
 * @return whole output of command call looking like '"0x02000002  0 131\n0x02000007  0 131\n0x0200000c  0 131\n"'
 */
std::string Manager::System_Call(const std::string &command) const
{
    char buffer[1024]; /* TO DO: This shouldn't be hardcoded. We should take the size of pipe, and dynamically allocate memory for buffer with that size. */
    std::string result = "";

    std::shared_ptr<FILE> pipe (popen(command.c_str(), "r"), pclose); //Why unique ptr not working??

    if (!pipe)
        throw std::runtime_error("popen() failed!");

    /* Append output from system call to string. */
    while (!feof(pipe.get()))
    {
        if (fgets(buffer, 1024, pipe.get()) != NULL)
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
std::set<int> Manager::Get_PIDs_from_Strings(std::vector<std::string> &input) const
{
    std::set<int> pid_numbers;

    for(auto it = input.begin(); it != input.end(); ++it)
    {
        std::string pid = "";

        /* First serie of characters from the end is a pid number. Save it, and repeat loop, until we get all PIDs. */
        for(std::string::reverse_iterator string_iterator = (*it).rbegin(); string_iterator != (*it).rend(); ++string_iterator)
        {
            if(isdigit(*string_iterator))
            {
                pid += *string_iterator;
            }
            else
            {
                std::reverse(pid.begin(), pid.end());
                int pid_nr = std::stoi(pid);
                pid_numbers.insert(pid_nr);
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

void Manager::Start()
{
    std::string command = "wmctrl -lp | grep -o '0x[0-9a-z]*\s*  [0-9] [0-9]\\{1,6\\}'";
    std::string system_call_result = System_Call(command);

    std::vector<std::string> strings_split = Split_Command_Output_to_Strings(system_call_result);
    std::set<int> pid_numbers = Get_PIDs_from_Strings(strings_split);
    std::vector<std::string> processes_names = Get_Processes_Names(pid_numbers);

    /* Create Items out of processes names */
    for(auto &process : processes_names)
    {
        Item item;
        item.name = process;

        /* TO DO: Add retrieving icons from system. */
        item.Set_Icon("/home/patryk/QT Projects/Manic LTime/HeavyRain.png");

        //std::this_thread::sleep_for(std::chrono::seconds(1));
        Add_Item(item);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    //Print_Elapsed_Time();

    Save_Statistics_to_File();
    Load_Statistics_from_File();

    /* Send signal to main window to show output on screen */

}





void Manager::Add_Item(const Item &item)
{
    //Can we make this function one-liner?
    Process_Statistics tmp;
    objects.emplace_back(std::pair<Item, Manager::Process_Statistics> (item, tmp));
}


void Manager::Print_Elapsed_Time() const
{
    for(auto &object : objects)
    {
        object.second.Stop_Counting_Time();
        qDebug() << QString::fromStdString(object.first.name) << ": " << QString::number(object.second.time_difference);
    }
}


/**
 * @brief Calls 'ps' syscall to get processes names based on their PIDs.
 * @param pid_numbers - PIDs to be translated to names of processes.
 * @return vector of processes names.
 */

std::vector<std::string> Manager::Get_Processes_Names(const std::set<int> &pid_numbers) const
{
    std::vector<std::string> processes_names;

    /* Call 'ps' syscall to get processes names based on their PIDs */
    for(auto it = pid_numbers.begin(); it != pid_numbers.end(); ++it)
    {
        /* TO DO: Add nicer way of calling function (got hint on e-mail) */
        std::string process_name = System_Call("ps -p " + std::to_string(*it) + " -o comm=");
        /* Remove new line from the end of string. */
        process_name.erase(std::remove(process_name.begin(), process_name.end(), '\n')); //We could have called .pop_back(), more efficient, but less readable.

        processes_names.emplace_back(std::move(process_name));
    }

    return processes_names;
}



void Manager::Save_Statistics_to_File()
{
    file_stats.open(path_to_file, std::fstream::out);
    if(!file_stats.is_open())
        throw std::ios_base::failure("Couldn't open a file in order to save statistics: " + path_to_file);

    for(auto &object : objects)
    {
        object.second.Stop_Counting_Time();
        object.second.Parse_Time();
        file_stats << object.first.name << " ::: " << object.second.total_hours << ":" << object.second.total_minutes << ":" << object.second.total_seconds << "\n";

        cout << std::fixed << object.second.time_difference << endl;
        QApplication::processEvents();
    }
}




void Manager::Process_Statistics::Parse_Time()
{

    Stop_Counting_Time();
    /* TO DO: Make asserts, that time_difference at the beginning of this function is of type chrono::seconds */

    /* If process was ON more than 60 seconds */
    if(time_difference >= 60.0)
    {
        /* If process was ON more than 1 hour */
        if(time_difference >= 3600.0)
        {
            total_seconds = Parse_Seconds();
            total_minutes = Parse_Minutes();
            total_hours = std::chrono::duration_cast<Process_Statistics::hours>(end_time - begin_time).count();
        }
        /* 1 - 59 MINUTES */
        else
        {
            total_seconds = Parse_Seconds();
            total_minutes = std::chrono::duration_cast<Process_Statistics::minutes>(end_time - begin_time).count();
            total_hours = 0;
        }
    }
    /* 0 - 59 SECONDS */
    else
    {
        total_seconds = time_difference;
        total_minutes = total_hours = 0;
    }
}



void Manager::Process_Statistics::Stop_Counting_Time()
{
    end_time = Process_Statistics::_clock::now();
    /* time_difference must be assigned in seconds! Some part of application are based on it. */
    time_difference = std::chrono::duration_cast<Process_Statistics::seconds>(end_time - begin_time).count();
}


int Manager::Process_Statistics::Parse_Minutes() const
{
    int minutes = std::chrono::duration_cast<Process_Statistics::minutes>(end_time - begin_time).count();
    while(minutes > 59)
    {
        minutes = minutes - 60;
    }
    return minutes;
}

int Manager::Process_Statistics::Parse_Seconds() const
{
    int seconds = std::chrono::duration_cast<Process_Statistics::seconds>(end_time - begin_time).count();
    while(seconds > 59)
    {
        seconds = seconds - 60;
    }
    return seconds;
}



void Manager::Load_Statistics_from_File()
{
    file_stats.open(path_to_file, std::fstream::in);
    if(!file_stats.is_open())
        throw std::ios_base::failure("Couldn't open a file in order to load statistics: " + path_to_file);
    std::string line;

    while(std::getline(file_stats, line))
    {
        cout << "Line: " << line << endl;
    }

}








