//Local includes
#include "Item.h"

//C++ includes
#include <fstream>
#include <iostream>

//POSIX includes
#ifdef __linux__
#include <dirent.h>
#endif

using std::cout;
using std::endl;

Item::Item(const std::string &_name) : name(_name), icon(std::unique_ptr<QPixmap>(nullptr))
{
    Load_Icon();
}

Item::Item(std::string &&_name) : name(std::move(_name)), icon(std::unique_ptr<QPixmap>(nullptr))
{
    Load_Icon();
}

Item::Item(const Item &rhs) : name(rhs.name)
{
    if(rhs.icon != nullptr)
    {
        icon = std::make_unique<QPixmap>();
        *icon = *rhs.icon;
    }
}

Item::Item(Item &&rhs) noexcept
{
    name = std::move(rhs.name);
    icon = std::move(rhs.icon);
}

void Item::Load_Icon()
{
#ifdef __linux__
    static const std::string icons_path = "/usr/share/pixmaps/";
    static const std::string icons_path_2 = "/usr/share/icons/hicolor/";
    static const std::string icons_path_3 = "/usr/share/icons/Humanity/";

    /* We will search for icons in variety of paths. Linux has them all over the place, sadly.*/
    std::vector<std::string> paths_to_icons = {icons_path, icons_path_2 + "32x32/apps/", icons_path_2 + "48x48/apps/", icons_path_2 + "64x64/apps/", icons_path_3 + "apps/32/", icons_path_3 + "apps/48/"};
    struct dirent *dir_struct;

    for(std::vector<std::string>::iterator path = paths_to_icons.begin(); path != paths_to_icons.end(); ++path)
    {
        std::unique_ptr<DIR, decltype(&closedir)> directory (opendir(path->c_str()), closedir);
        if(directory == NULL)
            continue;

        /* Search whole directory to find a file with the same name as observed process, with .png or .svg extension */
        while((dir_struct = readdir(directory.get())) != NULL)
        {
            if(strcmp(dir_struct->d_name, (name + ".png").c_str()) == 0 ||
                    strcmp(dir_struct->d_name, (name + ".svg").c_str()) == 0)
            {
                /* Load icon from file */
                icon = std::make_unique<QPixmap>(QString::fromStdString(*path + dir_struct->d_name));
                /* We want all icons to be of the same size */
                Scale_Icon(*icon);

                cout << "Found an icon! : " << dir_struct->d_name << endl;
                return;
            }
        }
    }
    cout << "Icon for name: " << name << " wasn't found." << endl;
    #endif
}

void Item::Scale_Icon(QPixmap &pixmap)
{
    pixmap = pixmap.scaled(QSize(32, 32), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}
