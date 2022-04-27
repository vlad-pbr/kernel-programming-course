// prog.c

#include <netinet/in.h>
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/stat.h>

#define BUFFER 4096
#define PORT 8080

struct download {
    int index;
    char* url;
    int bytes_downloaded;
    int fd;
};

void main(int argc, char *argv[]) {

    // define variables
    struct sockaddr_in address;
    int bytes_read;
    int server_recv_socket_fd;
    int server_recv_file_fd;
    struct stat server_recv_file_stat;
    int addrlen = sizeof(address);
    char buf[BUFFER];
    char *token;
    struct pollfd fds[2];
    struct download *downloads = malloc(0);
    int downloads_amt = 0;

    // set up polling for stdin
    fds[0].fd = 0;
    fds[0].events = POLLIN;

    // create new TCP socket
    fds[1].fd = socket(AF_INET, SOCK_STREAM, 0);

    // set up server socket polling
    fds[1].events = POLLIN;

    // make server listen on any address on given port
    address.sin_family = AF_INET; // TODO change to local?
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // bind socket to host
    int b = bind(fds[1].fd, (struct sockaddr*)&address, sizeof(address));
    printf("0 is no error: %d\n", b);

    // make server listen on socket
    int l = listen(fds[1].fd, 3);
    printf("0 is no error: %d\n", l);

    while(1) {

        // poll fds
        poll(fds, 2, -1);

        // handle stdin
        if(fds[0].revents & POLLIN) {

            // read stdin
            bytes_read = read(0, buf, BUFFER);
            buf[bytes_read] = '\0';

            // handle leave
            if (!strcmp(buf, "leave\n")) {
                break;
            }

            // handle show
            else if (!strcmp(buf, "show\n")) {

                // show downloads
                for(int i = 0; i < downloads_amt; i++) {
                    printf("%d %s %d\n", downloads[i].index, downloads[i].url, downloads[i].bytes_downloaded);
                }

            }

            // handle start / stop
            else {
                
                // get first word
                token = strtok(buf, " ");

                // handle start
                if (!strcmp(buf, "start")) {
                    
                    // allocate space for new download
                    downloads = realloc(downloads, sizeof(struct download) * ++downloads_amt);

                    // populate new download
                    downloads[downloads_amt-1].index = downloads_amt;
                    downloads[downloads_amt-1].url = strtok(NULL, " ");

                    printf("%s\n", downloads[downloads_amt-1].url);

                }

                // handle stop
                else {
                    printf("stopping\n");
                }

            }

        }

        // handle server
        if(fds[1].revents & POLLIN) {

            // accept and read single request
            server_recv_socket_fd = accept(fds[1].fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
            read(server_recv_socket_fd, buf, BUFFER);

            // get uri
            strtok(buf, " ");
            token = strtok(NULL, " ");
            token += 1;

            // send file TODO can be too large?
            stat(token, &server_recv_file_stat);
            server_recv_file_fd = open(token, O_RDONLY);
            sendfile(server_recv_socket_fd, server_recv_file_fd, NULL, server_recv_file_stat.st_size);

            // close fds
            close(server_recv_file_fd);
            close(server_recv_socket_fd);
        }

    }

    free(downloads);

    // TODO close server

    close(fds[1].fd);

    printf("bye\n");

}