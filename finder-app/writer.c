#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    // Initiates syslog with LOG_USER facility.
    openlog("[writer]", LOG_PID, LOG_USER);

    // Check for the correct number of arguments.
    if (argc != 3) {
        printf("Invalid number of arguments: %d provided, 2 expected\n", argc - 1);
        syslog(LOG_ERR, "Invalid number of arguments: %d provided, 2 expected\n", argc - 1);
        closelog();
        return 1;
    }

    const char *writefile = argv[1];
    const char *writestr = argv[2];

    syslog(LOG_DEBUG, "Writing \"%s\" to %s\n", writestr, writefile);

    int fd;
    fd = open(writefile, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (fd == -1) {
        printf("Failed to open file %s: %s\n", writefile, strerror(errno));
        syslog(LOG_ERR, "Failed to open file %s: %s\n", writefile, strerror(errno));
        closelog();
        return 1;
    }

    ssize_t bytes_to_write = strlen(writestr);
    ssize_t bytes_written = write(fd, writestr, bytes_to_write);

    if (bytes_written != bytes_to_write) {
        printf("File write incomplete, bytes written %ld/%ld\n", bytes_written, bytes_to_write);
        syslog(LOG_ERR, "File write incomplete, bytes written %ld/%ld\n", bytes_written, bytes_to_write);
        close(fd);
        closelog();
        return 1;
    }

    if (close(fd) == -1) {
        printf("Failed to close file %s: %s", writefile, strerror(errno));
        syslog(LOG_ERR, "Failed to close file %s: %s\n", writefile, strerror(errno));
        closelog();
        return 1;
    }

    printf("Successfully wrote string %s to file %s\n", writestr, writefile);
    syslog(LOG_DEBUG, "Successfully wrote string %s to file %s\n", writestr, writefile);
    closelog();

    return 0;
}