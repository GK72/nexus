#include "fs.h"

#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

int listdir(const char* path) {
    DIR* dir;

    dir = opendir(path);
    if (dir == NULL) {
        perror("Invalid path");
        return 1;
    }

    struct dirent* dp;
    struct stat st;
    while ((dp = readdir(dir))) {
        stat(dp->d_name, &st);
        char* type;
        time_t t = st.st_mtime;
        char timebuff[70];
        // TODO: does not work correctly when not the current dir
        if (S_ISDIR(st.st_mode)) {
            type = "DIR ";
        }
        else {
            type = "FILE";
        }
        strftime(timebuff, sizeof(timebuff), "%Y-%m-%d %H:%M:%S", gmtime(&t));
        printf("%s - %s %s\n", type, timebuff, dp->d_name);
    }

    closedir(dir);
    return 0;
}