// prog.c

#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/msg.h>

#define BUFFER_SIZE 256 + 10 + 1 + 1 // 256 (max file path length) + 10 (int max size as string) + 1 (space) + 1 (null terminator)
#define MSGTYPE 1

struct result {
    char name[BUFFER_SIZE];
    int words;
};

struct msgbuf {
   long mType;
   char mText[sizeof(struct result)];
};

void handleDirectory(char* path, int qId) {

    // define vars
    DIR* dir;
    struct dirent* entry;
    struct stat entry_stat;
    char* entry_path;
    int pipe_out_fds[2];
    char buffer[BUFFER_SIZE];
    int buffer_index;
    pid_t pid;
    int rc;
    struct result r;
    struct msgbuf msg;
    struct msqid_ds ds;

    // set same message type for all messages
    msg.mType = MSGTYPE;

    // iterate files in directory
    dir = opendir(path);
    for (entry = readdir(dir); entry != NULL; entry = readdir(dir)) {

        // ignore current and previous directories
        if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {

            // allocate memory for combined entry path and store
            entry_path = malloc(strlen(path) + strlen(entry->d_name) + 2);
            sprintf(entry_path, "%s/%s", path, entry->d_name);

            // stat current entry
            stat(entry_path, &entry_stat);

            // see if entry is a directory or file and handle accordingly
            if (S_ISDIR(entry_stat.st_mode) == 1) {

                if ((pid = fork()) == 0) {

                    // handle subdirectory
                    handleDirectory(entry_path, qId);

                    exit(0);
                }

                // wait for child to exit
                waitpid(pid, &rc, 0);

            } else {

                // set-up pipe
                pipe(pipe_out_fds);

                if ((pid = fork()) == 0) {

                    // copy appropriate fds to stdin and stdout and close the rest
                    dup2(pipe_out_fds[1], 1);
                    close(pipe_out_fds[0]);
                    close(pipe_out_fds[1]);

                    // run wc
                    execlp("wc", "wc", "-w", entry_path, (char *)NULL );
                }

                // wait for child to exit
                waitpid(pid, &rc, 0);
                close(pipe_out_fds[1]);

                // read from pipe
                read(pipe_out_fds[0], buffer, BUFFER_SIZE);
                close(pipe_out_fds[0]);

                // skip to space in output and null terminate
                for (buffer_index = 0; buffer[buffer_index] != ' '; buffer_index++);
                buffer[buffer_index] = '\0';

                // fill result struct
                strcpy(r.name, entry_path);
                r.words = atoi(buffer);

                // send result to queue
                strcpy(msg.mText, (char *)&r);
                msgsnd(qId, &msg, sizeof(msg.mText), 0);

                // wait for message to be received by the main process
                do {
                    msgctl(qId, IPC_STAT, &ds);
                } while(ds.msg_qnum != 0);

            }

            free(entry_path);
        }

    }

}

void main(int argc, char *argv[]) {

    // define vars
    pid_t pid;
    int rc;
    key_t qKey;
    int qId;
    struct msgbuf msg;
    struct result* results;
    int resultsAmt;
    float average;
    float variance;

    // initial results memory allocation
    results = malloc(0);

    // generate systemv ipc queue
    qKey = ftok("/tmp", 'z' );
    qId = msgget(qKey, IPC_CREAT | 0666 );

    // handle given directory
    if ((pid = fork()) == 0) {

        // handle provided dir
        handleDirectory(argv[1], qId);

        // close queue
        msgctl(qId, IPC_RMID, NULL);

        exit(0);
    }

    // prepare variables
    average = 0;
    variance = 0;
    resultsAmt = 0;

    // read until end of queue
    while(msgrcv(qId, &msg, sizeof(msg.mText), MSGTYPE, 0) != -1) {

        results = realloc(results, sizeof(struct result) * (resultsAmt + 1) );

        // cast message to result
        results[resultsAmt] = *(struct result *)msg.mText;

        // add to average
        average += results[resultsAmt++].words; 
    }

    // calculate average
    average /= resultsAmt;

    // calculate variance
    for (int i = 0; i < resultsAmt; i++) {
        variance += (results[i].words - average) * (results[i].words - average);
    }
    variance /= resultsAmt;

    // results
    for (int i = 0; i < resultsAmt; i++) {
        printf("name: %s, words: %d, diff from average: %f\n", results[i].name, results[i].words, results[i].words - average);
    }
    printf("\naverage: %f\nvariance: %f\n", average, variance);

    free(results);
}
