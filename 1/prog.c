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

#define BUFFER_SIZE 256

struct result {
    char name[BUFFER_SIZE];
    int words;
};

struct msgbuf {
   long mType;
   char mText[sizeof(struct result)];
};

void handleDirectory(char* path, int pipe_parent_dir_fds[2]) {

    // define vars
    DIR* dir;
    struct dirent* entry;
    struct stat entry_stat;
    char* entry_path;
    int entry_fd;
    int pipe_in_fds[2];
    int pipe_out_fds[2];
    char buffer[BUFFER_SIZE];
    ssize_t read_amt;
    pid_t pid;
    int rc;
    struct result r;

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

                    // close reading end of pipe
                    close(pipe_parent_dir_fds[0]);

                    // handle subdirectory
                    handleDirectory(entry_path, pipe_parent_dir_fds);

                    // close writing end of pipe
                    close(pipe_parent_dir_fds[1]);

                    exit(0);
                }

                // wait for child to exit
                waitpid(pid, &rc, 0);

            } else {

                // set-up pipes
                pipe(pipe_in_fds);
                pipe(pipe_out_fds);

                // write file contents to input side of pipe and close
                entry_fd = open(entry_path, O_RDONLY);
                sendfile(pipe_in_fds[1], entry_fd, NULL, entry_stat.st_size);
                close(pipe_in_fds[1]);

                if ((pid = fork()) == 0) {

                    // copy appropriate fds to stdin and stdout and close the rest
                    dup2(pipe_in_fds[0], 0);
                    dup2(pipe_out_fds[1], 1);
                    close(pipe_in_fds[0]);
                    close(pipe_in_fds[1]);
                    close(pipe_out_fds[0]);
                    close(pipe_out_fds[1]);
                    close(pipe_parent_dir_fds[0]);
                    close(pipe_parent_dir_fds[1]);

                    // run wc
                    execlp("wc", "wc", "-w", (char *)NULL );
                }

                // wait for child to exit
                waitpid(pid, &rc, 0);
                close(pipe_in_fds[0]);
                close(pipe_out_fds[1]);

                // read from pipe
                read_amt = read(pipe_out_fds[0], buffer, BUFFER_SIZE);
                close(pipe_out_fds[0]);
                buffer[read_amt] = '\0';

                // fill result struct
                strcpy(r.name, entry_path);
                r.words = atoi(buffer);

                // write result to parent pipe
                write(pipe_parent_dir_fds[1], &r, sizeof(struct result));

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
    long msgtype;
    struct msgbuf msg;
    struct result* results;
    int resultsAmt;
    float average;
    float variance;

    // initial results memory allocation
    results = malloc(0);

    // set the same message type for all messages
    msgtype = 1;

    // generate systemv ipc queue
    qKey = ftok("/tmp", 'z' );
    qId = msgget(qKey, IPC_CREAT | 0666 );

    // handle given directory
    if ((pid = fork()) == 0) {

        // define vars
        int pipe_dir_fds[2];

        // make pipe and pass to recursive function
        pipe(pipe_dir_fds);

        // handle provided dir
        handleDirectory(argv[1], pipe_dir_fds);

        // close writing end of pipe
        close(pipe_dir_fds[1]);

        // set type for each following message to be the same
        msg.mType = msgtype;

        // read each result and send to message queue
        while ( read(pipe_dir_fds[0], msg.mText, sizeof(msg.mText)) ) {
            msg.mType = 1;
            msgsnd(qId, &msg, sizeof(msg.mText), 0);
        }

        // close reading end of pipe
        close(pipe_dir_fds[0]);

        // close queue
        msgctl(qId, IPC_RMID, NULL);

        exit(0);
    }

    // prepare variables
    average = 0;
    variance = 0;
    resultsAmt = 0;

    // read until end of queue
    while(msgrcv(qId, &msg, sizeof(msg.mText), msgtype, 0) != -1) {

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
