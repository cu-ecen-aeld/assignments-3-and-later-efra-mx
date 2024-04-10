#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
//#define _POSIX_C_SOURCE 200809L
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
#include <sys/time.h>
#include <stdbool.h>
#include <pthread.h>
#include "linked-list.h"

#define PORT "9000"  // the port users will be connecting to

#define BACKLOG 10   // how many pending connection-> queue will hold
#define BUFFER_SIZE (1 << 16)
#define ALARM_PERIOD 10

struct connection_data {
    struct node *next;
    int id;
    sig_atomic_t connected;
    int sock;  // listen on sock_fd, new connection->on listener_fd
    pthread_t thread;
    pthread_mutex_t *lock;
    struct sockaddr_storage addr_info; // connector's address information
    char addr[INET6_ADDRSTRLEN];
    char *buffer;
};

volatile sig_atomic_t running = 0;
int sockfd = -1;
FILE *data_fptr;
char socket_data_filename[] = "/var/tmp/aesdsocketdata";
struct connection_data *connections;
pthread_mutex_t lock;
struct node *list = NULL;
int counter = 0;
timer_t timer_id;

void signal_handler(int signo){
    int saved_errno = errno;
    printf("signum: %d\n", signo);// debug
    switch (signo) {
        case SIGINT:
            printf("SIGINT handler\n");//debug
            break;
        case SIGTERM:
            printf("SIGTERM handler\n");//debug
            break;
        default: /*Should never get this case*/
            break;
    }
    running = 0;

    struct node *node = list;
    struct connection_data *connection;
    while (node != NULL) {
        connection = (struct connection_data *) node;
        connection->connected = 0;
        if (connection->sock)
            shutdown(connection->sock, SHUT_RDWR);
        node = node->next;
    }
    if (sockfd)
        shutdown(sockfd, SHUT_RDWR);
    sockfd = -1;
    if (connection)
        connection->sock = -1;

    errno = saved_errno;
}


void alarm_handler(int signo){
    int saved_errno = errno;
    char outstr[128];
    struct tm* tm_info;
    time_t t;
    struct tm *tmp;
    
    printf("signum: %d\n", signo);// debug
    switch (signo) {
        case SIGALRM:
            printf("SIGALRM handler\n");//debug
            break;
        default: /*Should never get this case*/
            return;
    }
   
    t = time(NULL);
    tmp = localtime(&t);
    if (tmp == NULL) {
        perror("localtime");
        return;
    }

    if (strftime(outstr, sizeof(outstr), "%a, %d %b %Y %T %z", tmp) == 0) {
        printf("strftime error");
        return;
    }

    pthread_mutex_lock(&lock);
    fprintf(data_fptr, "timestamp:%s\n", outstr);
    printf("<< timestamp:%s\n", outstr);
    alarm(ALARM_PERIOD);
    pthread_mutex_unlock(&lock);
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
    socklen_t sin_size;
    int yes=1;
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
//  0: connection->aborted
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


void *thread_func(void *ptr)
{
    int len = BUFFER_SIZE - 1;
    int rc = 0;
    struct connection_data *conn = (struct connection_data *)ptr;
    int error;
    int sock = conn->sock;
    char *buffer  = conn->buffer;

    conn->connected = 1;

    inet_ntop(conn->addr_info.ss_family,
        get_in_addr((struct sockaddr *)&conn->addr_info),
        conn->addr, sizeof(conn->addr));
    printf("server: got connection from %s\n", conn->addr);
    syslog(LOG_INFO, "Accepted connection from %s", conn->addr);
    do {
        pthread_mutex_lock(conn->lock);
        len = BUFFER_SIZE - 1;
        rc = receive_data(sock, buffer, len);
        if (rc == 1) {
            // write to file
            fprintf(data_fptr, "%s\n", buffer);
            printf("[%d]<< %s\n", sock, buffer);

            pthread_mutex_unlock(conn->lock);

            pthread_mutex_lock(conn->lock);
            fseek(data_fptr, 0, SEEK_SET);
            memset(buffer, 0, len);
            len = fread(buffer, 1, len, data_fptr);
            rc = send(sock, buffer, len, 0);
            if (rc <= 0) {
                conn->connected = 0;
            }
            printf("[%d]>> %s\n", sock,buffer);
        } else if (rc == 2) {
            // write to file
            fprintf(data_fptr, "%s", buffer);
        } else if (rc <= 0) {
            // error
            conn->connected = 0;
        }
        pthread_mutex_unlock(&lock);
        sleep(1);
    } while (conn->connected);

safe_exit:
    printf("thread[%d] socket[%d]: finishing\n", conn->id, conn->sock);
    if (sock >= 0) {
        close(sock);
        conn->sock = -1;
    }
    syslog(LOG_INFO, "Closed connection->from %s", conn->addr);
    error = rc;
    return ptr;
}

int run(void)
{
    socklen_t sin_size;
    struct itimerspec its;
    struct sigaction sa;
    int listener;
    int rc;
    struct connection_data *connection = NULL;
    struct node *node = NULL;
    char *buffer;
    struct sigevent sev;
    sigset_t mask;

    printf("server: waiting for connection...\n");

    buffer = (char *)malloc(BUFFER_SIZE);
    memset(buffer, 0, BUFFER_SIZE);
    if (!buffer) {
        rc = -1;
        printf("No memory");
        goto safe_exit;
    }
    openlog ("aesdsocket", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
    data_fptr = fopen(socket_data_filename, "w");
    fclose(data_fptr);
    data_fptr = fopen(socket_data_filename, "a+");

    alarm(ALARM_PERIOD);
    running = 1;
    while(running) {  // main accept() loop
        connection = (struct connection_data *)node_create(sizeof(struct connection_data));
        if (!connection) {
            rc = -1;
            printf("No memory for allocating a new connection");
            continue;
        }

        sin_size = sizeof(connection->addr_info);
        connection->sock = accept(sockfd, (struct sockaddr *)&connection->addr_info, &sin_size);
        if (connection->sock == -1) {
            perror("accept");
            node_destroy((struct node *)connection);
            continue;
        }
        printf("listen socket: %d\n", connection->sock);
        if (connections == NULL) {
            connections = connection;
            list = (struct node *)connections;
        } else {
            list_push(list, (struct node *)connection);
        }
        connection->lock = &lock;
        connection->buffer = buffer;
        connection->id = counter++;
        printf("thread[%d] socket[%d]: new\n", connection->id, connection->sock);
        pthread_create(&connection->thread, NULL, &thread_func, (void *) connection);
    }

safe_exit:
    while (list != NULL) {
        connection = (struct connection_data *) list;
        // wait for threads to finish
        pthread_join(connection->thread, NULL);
        node = list;
        list = list->next;
        node_destroy(node);
    }
    if (timer_id)
        timer_delete(timer_id);
    if (sockfd >= 0)
        close(sockfd);
    fclose(data_fptr);
    if (socket_data_filename) {
        unlink(socket_data_filename);
    }
    if (buffer)
        free(buffer);
    return 0;
}

int main(int argc, char *argv[])
{
    struct addrinfo *p;
    struct sigaction sa = {0};
    struct sigaction alarm = {0};
    int rv = 0;
    struct stat st = {0};
    int daemon_mode = 0;
    pid_t pid;

    if( argc == 2 ) {
        if (strcmp(argv[1], "-d") == 0) {
            daemon_mode = 1;
        }
    } else if (argc > 2) {
        printf("Too many arguments. Use -d to run as daemon.");
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

    alarm.sa_handler = alarm_handler;
    alarm.sa_flags = SA_RESTART;
    if (sigaction(SIGALRM, &alarm, NULL) == -1) {
        perror("alarm");
        exit(-1);
    }

    if (stat("/var/tmp", &st) == -1) {
        mkdir("/var/tmp", 0777);
    }
    rv = create_socket(&sockfd, &p);
    if (rv != 0 || p == NULL || sockfd < 0)  {
        fprintf(stderr, "server: failed to open thr socket\n");
        rv = -1;
        goto finish;
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        rv = -1;
        goto finish;
    }

    if (daemon_mode) {
        pid = fork();
        if (pid < 0) {
            perror("fork fail");
            rv = -1;
        } else if (pid == 0) {
            rv = run();
        } else {
            // parent
            printf("Running as deamon: %d\n", pid);
        }
    } else {
        rv = run();
    }

finish:
    if (sockfd >= 0)
        close(sockfd);
    return rv;
}

