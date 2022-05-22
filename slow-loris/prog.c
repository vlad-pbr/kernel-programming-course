// prog.c

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/msg.h>
#include <string.h>
#include <signal.h>
#include <netdb.h>
#include <arpa/inet.h>

#define BUFFER_LEN 256
#define MSGTYPE 1
#define QID msgget(ftok("/tmp", 'z' ), IPC_CREAT | 0666 )

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

    // create connection
    // send byte at interval in while true
    // if lost - reestablish and while again

    // define variables
    int socket_fd;
    struct sockaddr_in connect_addr;

    // prepare connection
    connect_addr.sin_family = AF_INET;
    connect_addr.sin_port = htons(port);
    inet_aton(ip, &connect_addr.sin_addr.s_addr);
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    while (1) {

        send_message(id, "attempting to connect to remote host...");

        // connect to server
        if (connect(socket_fd, (struct sockaddr*)&connect_addr, sizeof(connect_addr)) == -1) {
            send_message(id, "could not connect to remote host");
            break;
        }

        send_message(id, "connected to remote host");

        while (1) {
            // TODO keep connection
        }

    }

    return 1;
}

int main(int argc, char *argv[]) {

    // define variables
    struct msgbuf msg;
    struct log log;
    struct msqid_ds ds;
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

    // spawn workers
    printf("[main]: spawning workers...\n");
    for (int worker_id = 0; worker_id < atoi(argv[3]); worker_id++) {

        // spawn worker process
        if ( (pid = fork()) == 0) {
            exit(worker(worker_id + 1, argv[1], atoi(argv[2]), atoi(argv[3]), atoi(argv[4])));
        } else {
            worker_pids[worker_id] = pid;
        }

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