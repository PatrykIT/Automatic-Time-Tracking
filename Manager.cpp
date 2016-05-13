#include "Manager.h"

#include <QDebug>

#include <iostream>
#include "ui_mainwindow.h"

using namespace std;

std::map<Item::Time_Statistics, Item> Manager::objects;

Manager::Manager(Ui::MainWindow &ui, QObject *parent) : QObject(parent), ui_m(&ui)
{

}


Manager::Manager(const Manager &other)
{
    objects = other.objects;
    //std::cout << objects[] //How to cout Time Statistics? :D
}


Manager::~Manager()
{

}

/**
 * @brief System_Call
 * @param command - command name to be called
 * @return output of command call
 */
std::string Manager::System_Call(const string &command)
{
    char buffer[256];
    std::string result = "";

    std::shared_ptr<FILE> pipe (popen(command.c_str(), "r"), pclose); //Why unique ptr not working??

    if (!pipe)
        throw std::runtime_error("popen() failed!");

    /* Append output from system call to string. */
    while (!feof(pipe.get()))
    {
        if (fgets(buffer, 256, pipe.get()) != NULL)
            result += buffer;
    }

    return result;
}

/**
 * @brief Processes input that looks like '0x03000001  0 1883' and gets out of it last serie of digits - which is PID number of a process.
 * @param input - vector of strings, in which each string is a separate line looking like '0x03000001  0 1883'.
 * @return set of PIDs
 */
std::set<int> Manager::Get_PIDs_from_strings(std::vector<string> &input)
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
 * @return vector of split output - one string as one line
 */
std::vector<std::string> Manager::Split_Output_to_Strings(const string &input)
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
    std::string result = System_Call(command);

    std::vector<std::string> strings_split = Split_Output_to_Strings(result);
    std::set<int> pid_numbers = Get_PIDs_from_strings(strings_split);

    std::vector<std::string> processes_names;

    /* Call ps syscall to get processes names based on their PIDs */
    for(auto it = pid_numbers.begin(); it != pid_numbers.end(); ++it)
    {
        /* TO DO: Add nicer way of calling function (got hint on e-mail) */
        processes_names.emplace_back(System_Call("ps -p " + std::to_string(*it) + " -o comm="));
    }

    /* Create Items out of processes names */
    for(auto &process : processes_names)
    {
        Item *item = new Item; //Is freed at the end of the program
        item->name = QString::fromStdString(process);

        /* TO DO: Add retrieving icons from system. */
        item->Set_Icon("/home/patryk/QT Projects/Manic LTime/HeavyRain.png");

        item->time_statistics.total_hours = item->time_statistics.total_minutes = item->time_statistics.total_seconds = 0;
        Add_Item(*item);
    }

    /* Show output on screen */



}





void Manager::Add_Item(const Item &item)
{
    objects.insert(make_pair(item.time_statistics, item));
}
