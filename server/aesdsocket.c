#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <stdbool.h>

#define PORT 9000
#define DATA_FILE "/var/tmp/aesdsocketdata"
#define BUFFER_SIZE 1024

int socket_fd = -1;
int client_fd = -1;
volatile sig_atomic_t signal_exit = 0;

void handle_signal(int /*signal*/) {
    syslog(LOG_INFO, "Caught signal, exiting");
    signal_exit = 1;

    if (socket_fd != -1) {
        // terminate all connections and close socket to free resources
        shutdown(socket_fd, SHUT_RDWR);
        close(socket_fd);
    }
}

void send_file_content(int client_socket) {
    FILE *file = fopen(DATA_FILE, "r");
    if (file == NULL) {
        syslog(LOG_ERR, "Failed to open data file %s: %s", DATA_FILE, strerror(errno));
        return;
    }
    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        ssize_t bytes_sent = send(client_socket, buffer, bytes_read, 0);
        if (bytes_sent == -1) {
            syslog(LOG_ERR, "Failed to send data to client: %s", strerror(errno));
            break;
        }
    }
    fclose(file);
}

// Helper to daemonize the process
void daemonize() {
    // 1. Fork off the parent process
    pid_t pid = fork();
    
    // An error occurred
    if (pid < 0) {
        syslog(LOG_ERR, "Forking Daemon process failed");
        exit(EXIT_FAILURE);
    }
    
    // Success: Let the parent terminate
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }
    
    // On success: The child process becomes the daemon
    
    // 2. Create a new SID for the child process
    if (setsid() < 0) {
        syslog(LOG_ERR, "setsid failed");
        exit(EXIT_FAILURE);
    }
    
    // 3. Change the current working directory
    if (chdir("/") < 0) {
        syslog(LOG_ERR, "chdir failed");
        exit(EXIT_FAILURE);
    }
    
    // 4. Redirect standard files to /dev/null
    // This prevents the daemon from trying to write to the terminal
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    
    open("/dev/null", O_RDONLY); // 0 = stdin
    open("/dev/null", O_WRONLY); // 1 = stdout
    open("/dev/null", O_RDWR);   // 2 = stderr
}

int main(int argc, char *argv[]) {
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len;
    int reused = 1;
    bool daemon_mode = false;

    if (argc > 1 && strcmp(argv[1], "-d") == 0) {
        daemon_mode = true;
    }

    // set up syslog
    openlog("aesdsocket", LOG_PID, LOG_USER);

    // set up signal handlers
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;

    if (sigaction(SIGINT, &sa, NULL) == -1 || sigaction(SIGTERM, &sa, NULL) == -1) {
        syslog(LOG_ERR, "Failed to set up signal handlers: %s", strerror(errno));
        return -1;
    }

    // create socket
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        syslog(LOG_ERR, "Failed to create socket: %s", strerror(errno));
        return -1;
    }

    // set socket options - this allows a server  to restart immediately after a crash
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &reused, sizeof(reused)) == -1) {
        syslog(LOG_ERR, "Failed to set socket options: %s", strerror(errno));
        close(socket_fd);
        return -1;
    }

    // bind socket to port 9000
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        syslog(LOG_ERR, "Failed to bind socket: %s", strerror(errno));
        close(socket_fd);
        return -1;
    }

    if (listen(socket_fd, 5) == -1) {
        syslog(LOG_ERR, "Failed to listen on socket: %s", strerror(errno));
        close(socket_fd);
        return -1;
    }

    if (daemon_mode) {
        syslog(LOG_INFO, "Running in daemon mode");
        daemonize();
    }

    while(!signal_exit) {
        client_addr_len = sizeof(client_addr);
        client_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_fd == -1) {
            if (errno == EINTR && signal_exit) {
                break; // exit loop on signal
            }
            syslog(LOG_ERR, "Failed to accept connection due to interruption: %s", strerror(errno));
            continue;
        }

        // Log client ip.
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        syslog(LOG_INFO, "Accepted connection from %s", client_ip);

        char *packet_buffer = NULL; // holds a packet
        size_t current_len = 0;
        char temp_buffer[BUFFER_SIZE]; // holds a chunk of a packet at each recv
        ssize_t bytes_received;

        while (!signal_exit && (bytes_received = recv(client_fd, temp_buffer, sizeof(temp_buffer), 0)) > 0) {
            char *new_ptr = realloc(packet_buffer, current_len + bytes_received + 1);
            if (new_ptr == NULL) {
                syslog(LOG_ERR, "Memory allocation failed: %s", strerror(errno));
                free(packet_buffer);
                break;
            }
            packet_buffer = new_ptr;
            memcpy(packet_buffer + current_len, temp_buffer, bytes_received);
            current_len += bytes_received;
            packet_buffer[current_len] = '\0';

            char *newline_loc = memchr(packet_buffer, '\n', current_len);
            if (newline_loc != NULL) {
                FILE *file = fopen(DATA_FILE, "a");
                if (file == NULL) {
                    syslog(LOG_ERR, "Failed to open data file %s: %s", DATA_FILE, strerror(errno));
                    continue;
                }
                size_t bytes_written = fwrite(packet_buffer, 1, current_len, file);
                if (bytes_written != current_len) {
                    syslog(LOG_ERR, "Failed to write complete packet to file: %s", strerror(errno));
                }

                fclose(file);

                // Echo back full file content to client
                send_file_content(client_fd);

                // Reset packet buffer for next packet
                free(packet_buffer);
                packet_buffer = NULL;
                current_len = 0;
            }
        }

        if (packet_buffer) {
            free(packet_buffer);
        }

        close(client_fd);
        client_fd = -1;
        syslog(LOG_INFO, "Closed connection from %s", inet_ntoa(client_addr.sin_addr));
    }

    if (socket_fd != -1) {
        close(socket_fd);
    }
    if (client_fd != -1) {
        close(client_fd);
    }

    remove(DATA_FILE);

    syslog(LOG_INFO, "Exiting");
    closelog();
    return 0;
}