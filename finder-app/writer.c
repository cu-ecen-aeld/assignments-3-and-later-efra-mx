#include <stdio.h>
#include <string.h>
#include <syslog.h>

int main (int argc, char *argv[])
{
    const char *writefile_path = argv[1];
    const char *writestr = argv[2];
    FILE *writefile;

    openlog(NULL, LOG_PID, LOG_USER);

    if (argc < 3) {
        printf("ERROR: This script requires two arguments:\n"
               "Usage:\n"
               "\t  %s FILE STRING\n"
               "FILE    path to a file on the filesystem\n"
               "STRING  text string which will be written FILE\n",
               argv[0]);
        syslog(LOG_ERR, "%s requires two arguments", argv[0]);

        return 1;
    }

    writefile = fopen(writefile_path,"w");
    if (writefile == NULL) {
        printf("The file %s could not be created", writefile_path);
        syslog(LOG_ERR, "The file %s could not be created", writefile_path);
        return 1;
    }

    fprintf(writefile, "%s\n", writestr);
    fclose(writefile);

    syslog(LOG_DEBUG, "Writing %s to %s", writestr, writefile_path);
    return 0;
}

