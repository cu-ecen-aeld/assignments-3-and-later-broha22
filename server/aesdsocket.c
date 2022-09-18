#include <stdio.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <poll.h>
#include <syslog.h>
#include <netdb.h>
#include <signal.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#define BUFFER_SIZE sizeof(char)*1024*1024

volatile sig_atomic_t loop;

static void signal_handler (int signal_number) {
    if (signal_number == SIGINT || signal_number == SIGTERM) {
        loop = 0;
    }
}

int main (int argc, char *argv[]) {
    struct addrinfo *rslt, *rp;
    struct sockaddr *client_addr;
    int sock_fd;
    char *buffer;
    struct sigaction action_h;
    loop = 1;
    memset(&action_h,0,sizeof(struct sigaction));
    action_h.sa_handler = signal_handler;
    
    sigaction(SIGINT, &action_h, NULL);
    sigaction(SIGTERM, &action_h, NULL);

    buffer = malloc(BUFFER_SIZE);
    memset(buffer,'\0',BUFFER_SIZE);
    if (buffer == NULL) {
        return -1;
    } 
    openlog("", 0, LOG_USER);
    FILE *io_file;
    io_file = fopen("/var/tmp/aesdsocketdata", "w+r");
    // syslog(LOG_DEBUG, "Writing %s to %s\n", argv[2], argv[1]);
    struct addrinfo hints;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
    hints.ai_flags = SOCK_STREAM;
    hints.ai_addr = NULL;
    hints.ai_canonname = NULL;
    hints.ai_next = NULL;

    int addr_info = getaddrinfo(NULL, "9000", &hints, &rslt);
    if (addr_info < 0) {
        free(buffer);
        closelog();
        return -1;
    }
    for (rp = rslt; rp != NULL; rp = rp->ai_next) {
        sock_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sock_fd < 0) {
            continue;
        }
        int enable = 1;
        setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &enable, (socklen_t)sizeof(int));
        setsockopt(sock_fd, SOL_SOCKET, SO_REUSEPORT, &enable, (socklen_t)sizeof(int));

        if (bind(sock_fd, rp->ai_addr, rp->ai_addrlen) == 0) break;
    }
    freeaddrinfo(rslt);

    if (rp == NULL) {
        free(buffer);
        closelog();
        return -1;
    }
    if (argc > 1 && strcmp(argv[1], "-d") == 0) {
        int fork_v = fork();
        if (fork_v < 0) {
            free(buffer);
            closelog();
            return -1;
        } else if (fork_v > 0) {
            usleep(50000);
            return 0;
        } else {
            setsid();
            fork_v = fork();
            if (fork_v < 0) {
                free(buffer);
                closelog();
                return -1;
            } else if (fork_v > 0) {
                return 0;
            }
        }
    }
    listen(sock_fd, 5);

    struct pollfd polling_v;
    polling_v.fd = sock_fd;
    polling_v.events  = POLLIN | POLLOUT;
    polling_v.revents = 0;


    while (loop) {
        fd_set sock_fd_set;
        FD_ZERO(&sock_fd_set);
        FD_SET(sock_fd, &sock_fd_set);
        if (poll(&polling_v, (nfds_t)1, 100) <= 0) {
            continue;
        }
        socklen_t clilen = (socklen_t)sizeof(struct sockaddr);
        client_addr = malloc((size_t)clilen);
        int n_sock_fd = accept(sock_fd, client_addr, &clilen);
        if (n_sock_fd < 0) {
            free(buffer);
            free(client_addr);
            closelog();
            return -1;
        }
        syslog(LOG_DEBUG, "Accepted connection from %d\n", ((struct sockaddr_in*)client_addr)->sin_addr.s_addr);
        memset(buffer,'\0',BUFFER_SIZE);    
        size_t size = recv(n_sock_fd, buffer, BUFFER_SIZE, 0);
        fseek(io_file, 0, SEEK_END);
        fwrite(buffer, 1, strlen(buffer), io_file);
        fseek(io_file, 0, SEEK_END);
        long fsize = ftell(io_file);
        fseek(io_file, 0, SEEK_SET);
        memset(buffer,'\0',BUFFER_SIZE);
        fread(buffer, fsize, 1, io_file);
        send(n_sock_fd, buffer, fsize, 0);
        close(n_sock_fd);
        syslog(LOG_DEBUG, "Closed connection from %d\n", ((struct sockaddr_in*)client_addr)->sin_addr.s_addr);
        free(client_addr);
    }
    free(buffer);
    close(sock_fd);
    fclose(io_file);
    remove("/var/tmp/aesdsocketdata");
    closelog();
    return 0;
}