#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <algorithm>

#include "scanner.h"

void FileScanner::scan(string path, vector<File>& result)
{
    DIR* d = opendir(path.c_str());
    if (!d)
        return;

    while (1) {
        struct dirent* dentry = readdir(d);
        if (!dentry)
            break;
        // Skip . and ..
        if (dentry->d_name[0] == '.' &&
            ((dentry->d_name[1] == '.' && dentry->d_name[2] == 0) ||
             dentry->d_name[1] == 0))
            continue;

        string fullpath = path + "/" + dentry->d_name;

        switch (dentry->d_type) {
        case DT_REG:
            result.push_back(File(fullpath));
            break;
        case DT_DIR:
            if (context.exclude_dirs.count(dentry->d_name))
                break;
            scan(fullpath,result);
            {
                struct winsize w;
                ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
                int col = w.ws_col - 15;
                printf("%10lu %s\033[0K\n\033[1A", result.size(),
                       fullpath.size() > col ? fullpath.substr(0,col ).c_str() : fullpath.c_str());
                break;
            }
        }
    }
    closedir(d);
    return;
}
