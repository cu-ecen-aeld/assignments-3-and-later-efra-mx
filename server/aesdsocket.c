#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <syslog.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <stdbool.h>

#define PORT "9000"  // the port users will be connecting to

#define BACKLOG 10   // how many pending connections queue will hold
#define BUFFER_SIZE 1024

volatile sig_atomic_t running = 0;
volatile sig_atomic_t connected = 0;
int sockfd = -1;
FILE *data_fptr;
int listener_fd;  // listen on sock_fd, new connection on listener_fd
char socket_data_filename[] = "/var/tmp/aesdsocketdata";


void signal_handler(int signo){
    int saved_errno = errno;
    printf("signum: %d", signo);// debug
    switch (signo) {
        case SIGINT:
            printf("SIGINT handler");//debug
            break;
        case SIGTERM:
            printf("SIGTERM handler");//debug
            break;
        default: /*Should never get this case*/
            break;
    }
    running = 0;
    connected = 0;
    if (sockfd)
        shutdown(sockfd, SHUT_RDWR);
    if (listener_fd)
        shutdown(listener_fd, SHUT_RDWR);
    sockfd = -1;
    sockfd = -1;

    errno = saved_errno;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int create_socket(int *socketfd, struct addrinfo **p)
{
    struct addrinfo hints, *servinfo;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int ret = 0;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((ret = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
        return -1;
    }

    // loop through all the results and bind to the first we can
    for(*p = servinfo; *p != NULL; *p = (*p)->ai_next) {
        if ((sockfd = socket((*p)->ai_family, (*p)->ai_socktype,
                (*p)->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            ret = -1;
            goto safe_exit;
        }

        if (bind(sockfd, (*p)->ai_addr, (*p)->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }
    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
    }

safe_exit:
    freeaddrinfo(servinfo); // all done with this structure
    return ret;
}

// Return values:
//  0: connection aborted
//  1: message completed
//  2: message incompleted
int receive_data(int sock, char *buffer, int buffer_len) {
    int len;
    int rc = 0;

    memset(buffer, 0, buffer_len);
    len = recv(sock, (void *)buffer, buffer_len - 1, 0);
    if (len < 0) {
        return -errno;
    }
    if (len == 0) {
        return 0;
    }
    if (buffer[len - 1] == '\n') {
        buffer[len -1] = '\0';
        return 1;
    }
    // incomplete message, no '\n'
    return 2;
}


int process(int sock) {
    char *buffer;
    int len = BUFFER_SIZE - 1;
    int rc = 0;

    buffer = (char *)malloc(BUFFER_SIZE);
    if (!buffer) {
        return -1;
    }
    connected = 1;

    do {
        rc = receive_data(sock, buffer, len);
        if (rc == 1) {
            // write to file
            fprintf(data_fptr, "%s\n", buffer);
            printf("DATA: %s\n", buffer);
        } else if (rc == 2) {
            // write to file
            fprintf(data_fptr, "%s", buffer);
        } else if (rc <= 0) {
            // error
            connected = 0;
        }
    } while (connected);

safe_exit:
    if (buffer)
        free(buffer);
    return rc;
}

int main(void)
{
    struct addrinfo *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;
    struct stat st = {0};

    if (stat("/var/tmp", &st) == -1) {
        mkdir("/var/tmp", 0700);
    }
    rv = create_socket(&sockfd, &p);
    if (rv != 0 || p == NULL || sockfd < 0)  {
        fprintf(stderr, "server: failed to open thr socket\n");
        exit(-1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(-1);
    }

    sa.sa_handler = signal_handler;
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        exit(-1);
    }
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("sigaction");
        exit(-1);
    }

    printf("server: waiting for connections...\n");

    openlog ("aesdsocket", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
    running = 1;
    while(running) {  // main accept() loop
        sin_size = sizeof their_addr;
        listener_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (listener_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);
        printf("server: got connection from %s\n", s);
        syslog(LOG_INFO, "Accepted connection from %s", s);
        data_fptr = fopen(socket_data_filename, "a");

        process(listener_fd);

        if (listener_fd >= 0)
            close(listener_fd);
        fclose(data_fptr);
        syslog(LOG_INFO, "Closed connection from %s", s);
    }

    if (sockfd >= 0)
        close(sockfd);
    if (socket_data_filename) {
        unlink(socket_data_filename);
    }
    return 0;
}

