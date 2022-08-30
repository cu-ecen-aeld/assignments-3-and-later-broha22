#include <stdio.h>
#include <syslog.h>

int main (int argc, char *argv[]) {
    openlog("", 0, LOG_USER);

    if (argc < 3) {
        syslog(LOG_ERR, "Not Enough arguments specified\n");
        closelog();
        return 1;
    }
    FILE *io_file;
    io_file = fopen(argv[1], "w");
    syslog(LOG_DEBUG, "Writing %s to %s\n", argv[2], argv[1]);
    fprintf(io_file, "%s", argv[2]);
    fclose(io_file);
    closelog();
    return 0;
}