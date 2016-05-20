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

Manager::Process_Statistics::Process_Statistics() : total_hours(0.0), total_minutes(0.0), total_seconds(0.0),
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
std::string Manager::System_Call(const std::string &command)
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
std::set<int> Manager::Get_PIDs_from_Strings(std::vector<std::string> &input)
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
std::vector<std::string> Manager::Split_Command_Output_to_Strings(const std::string &input)
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
    }

    Print_Elapsed_Time();


    /* Send signal to main window to show output on screen */

}





void Manager::Add_Item(const Item &item)
{
    //Can we make this function one-liner?
    Process_Statistics tmp;
    objects.emplace_back(std::pair<Item, Manager::Process_Statistics> (item, tmp));
}


void Manager::Print_Elapsed_Time()
{
    for(auto &object : objects)
    {
        object.second.end_time = Process_Statistics::_clock::now();
        object.second.time_difference = std::chrono::duration_cast<Process_Statistics::seconds>(object.second.end_time - object.second.begin_time).count();

        qDebug() << QString::fromStdString(object.first.name) << ": " << QString::number(object.second.time_difference);
    }
}


/**
 * @brief Calls 'ps' syscall to get processes names based on their PIDs.
 * @param pid_numbers - PIDs to be translated to names of processes.
 * @return vector of processes names.
 */

std::vector<std::string> Manager::Get_Processes_Names(const std::set<int> &pid_numbers)
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







