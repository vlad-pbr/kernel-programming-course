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

#define BUFFER_SIZE 256

// void handleFile(char* path) {
//     execlp("wc", "wc", "-w", (char *)NULL );
// }

void handleDirectory(char* path) {

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
    int word_count;
    pid_t pid;
    int rc;

    // iterate files in directory
    dir = opendir(path);
    for (entry = readdir(dir); entry != NULL; entry = readdir(dir)) {

        // sleep(1);

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
                    handleDirectory(entry_path);
                }

                // wait for child to exit
                waitpid(pid, &rc, 0);

            } else {

                printf("name: %s, file\n", entry_path);

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

                    // run wc
                    execlp("wc", "wc", "-w", (char *)NULL );
                }

                // wait for child to exit
                waitpid(pid, &rc, 0);
                close(pipe_in_fds[0]);
                close(pipe_out_fds[1]);

                // read from pipe
                read_amt = read(pipe_out_fds[0], buffer, BUFFER_SIZE);
                buffer[read_amt] = '\0';

                // parse amount of words to integer
                word_count = atoi(buffer);
                printf("%d\n", word_count);

            }

        }

    }

}

void main(int argc, char *argv[]) {

    // define vars
    pid_t pid;
    int rc;

    // handle given directory
    if ((pid = fork()) == 0) {
        handleDirectory(argv[1]);
    }

    waitpid(pid, &rc, 0);

}