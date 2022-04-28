// prog.c

#include <stdio.h>
#include <unistd.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <netdb.h>

#define BUFFER 4096
#define PORT 80

struct download {
    int index;
    char* url;
    int bytes_downloaded;
    int socket_fd;
    int file_fd;
};

void main(int argc, char *argv[]) {

    // define variables
    struct sockaddr_in connect_addr;
    int bytes_read;
    int stop_index;
    char buf[BUFFER];
    char *token;
    char *url;
    char *filename;
    int terminated;
    char request[BUFFER];
    char *request_hostname;
    struct pollfd fds[1];
    int poll_timeout_ms = 0;
    struct download *downloads = malloc(0);
    struct pollfd *downloads_poll_fds = malloc(0);
    int downloads_amt = 0;

    // set up polling for stdin
    fds[0].fd = 0;
    fds[0].events = POLLIN;

    while(1) {

        // poll fds
        poll(fds, 1, poll_timeout_ms);

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
                    downloads_poll_fds = realloc(downloads_poll_fds, sizeof(struct pollfd) * downloads_amt);

                    // parse args
                    url = strtok(NULL, " ");
                    filename = strtok(NULL, " ");
                    filename[strlen(filename) - 1] = '\0';

                    // populate new download
                    downloads[downloads_amt-1].index = downloads_amt;
                    downloads[downloads_amt-1].url = malloc(strlen(url) + 1);
                    strcpy(downloads[downloads_amt-1].url, url);
                    downloads[downloads_amt-1].socket_fd = socket(AF_INET, SOCK_STREAM, 0);
                    downloads[downloads_amt-1].file_fd = open(filename, O_WRONLY | O_CREAT, 0644);
                    downloads[downloads_amt-1].bytes_downloaded = 0;

                    // populate download pollfd
                    downloads_poll_fds[downloads_amt-1].fd = downloads[downloads_amt-1].socket_fd;
                    downloads_poll_fds[downloads_amt-1].events = POLLIN;

                    // copy url for parsing
                    url = malloc(strlen(downloads[downloads_amt-1].url) + 1);
                    strcpy(url, downloads[downloads_amt-1].url);

                    // get hostname
                    strtok(url, "/");
                    request_hostname = strtok(NULL, "/");

                    // prepare connection, resolve hostname and set address
                    connect_addr.sin_family = AF_INET;
                    connect_addr.sin_port = htons(PORT);
                    connect_addr.sin_addr = *(((struct in_addr **)gethostbyname(request_hostname)->h_addr_list)[0]);

                    // connect to server socket
                    connect(downloads[downloads_amt-1].socket_fd, (struct sockaddr*)&connect_addr, sizeof(connect_addr));

                    // prepare request
                    sprintf(request, "GET /%s HTTP/1.0\r\nHost: %s\r\n\r\n\r\n", strtok(NULL, " "), request_hostname);

                    // send download request
                    send(downloads[downloads_amt-1].socket_fd, request, strlen(request), 0);

                    free(url);
                }

                // handle stop
                else if (!strcmp(buf, "stop")) {

                    // parse index
                    stop_index = atoi(strtok(NULL, " "));
                    
                    // close fds
                    close(downloads[stop_index-1].file_fd);
                    close(downloads[stop_index-1].socket_fd);
                }

            }

        }

        // poll download fds
        poll(downloads_poll_fds, downloads_amt, poll_timeout_ms);

        // handle downloads
        for (int i = 0; i < downloads_amt; i++) {

            terminated = 0;

            // if download was stopped manually
            if(fcntl(downloads_poll_fds[i].fd, F_GETFD) != 0) {
                terminated = 1;
            }

            // if received input from socket
            if (terminated == 0 && downloads_poll_fds[i].revents & POLLIN) {

                // read from socket
                bytes_read = recv(downloads[i].socket_fd, &buf, BUFFER, 0);

                // 0 means EOF (which still triggers POLLIN) - close the socket
                if (bytes_read == 0) {
                    terminated = 1;
                }
                
                // write byte to file
                else {
                    write(downloads[i].file_fd, &buf, bytes_read);
                    downloads[i].bytes_downloaded += bytes_read;
                }
            }

            // remove download
            if (terminated) {

                close(downloads[i].file_fd);
                close(downloads[i].socket_fd);
                free(downloads[i].url);

                // remove download from downloads
                for(int j = 0; j < downloads_amt; j++) {
                    downloads[j] = downloads[j+1];
                    downloads_poll_fds[j] = downloads_poll_fds[j+1];
                }
                downloads = realloc(downloads, sizeof(struct download) * --downloads_amt);
                downloads_poll_fds = realloc(downloads_poll_fds, sizeof(struct pollfd) * downloads_amt);
                i--;
            }

        }

    }

    // release memory
    free(downloads);
    free(downloads_poll_fds);
}