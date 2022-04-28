// prog.c

#include <arpa/inet.h>
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

struct upload {
    int file_fd;
    int socket_fd;
};

struct download {
    int index;
    char* url;
    int bytes_downloaded;
    int socket_fd;
    int file_fd;
};

void remove_upload(struct upload *uploads, int len, int index) {
    for (int i = index; i < len; i++) {
        uploads[i] = uploads[i+1];
    }
}

void main(int argc, char *argv[]) {

    // define variables
    struct sockaddr_in address;
    struct sockaddr_in connect_addr;
    int bytes_read;
    int addrlen = sizeof(address);
    char buf[BUFFER];
    char *token;
    char byte;
    struct pollfd fds[2];
    int poll_timeout_ms = 500;
    struct download *downloads = malloc(0);
    struct upload *uploads = malloc(0);
    struct pollfd *uploads_poll_fds = malloc(0);
    struct pollfd *downloads_poll_fds = malloc(0);
    int downloads_amt = 0;
    int uploads_amt = 0;

    // set up polling for stdin
    fds[0].fd = 0;
    fds[0].events = POLLIN;

    // create new TCP socket
    fds[1].fd = socket(AF_INET, SOCK_STREAM, 0);

    // set up server socket polling
    fds[1].events = POLLIN;

    // make server listen on any address on given port
    address.sin_family = AF_INET;
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
        poll(fds, 2, poll_timeout_ms);

        // poll download fds
        poll(downloads_poll_fds, downloads_amt, poll_timeout_ms);

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
                    printf("%d %s %d\n", downloads[i].index, downloads[i].url, downloads[i].bytes_downloaded); // TODO why url invalid?
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

                    // populate new download
                    downloads[downloads_amt-1].index = downloads_amt;
                    downloads[downloads_amt-1].url = strtok(NULL, " ");
                    downloads[downloads_amt-1].socket_fd = socket(AF_INET, SOCK_STREAM, 0);
                    downloads[downloads_amt-1].file_fd = open(strtok(NULL, " "), O_WRONLY | O_CREAT, 0644);
                    downloads[downloads_amt-1].bytes_downloaded = 0;

                    // populate download pollfd
                    downloads_poll_fds[downloads_amt-1].fd = downloads[downloads_amt-1].socket_fd;
                    downloads_poll_fds[downloads_amt-1].events = POLLIN;

                    printf("%d %s %d %d %d\n", downloads[downloads_amt-1].index, downloads[downloads_amt-1].url, downloads[downloads_amt-1].socket_fd, downloads[downloads_amt-1].file_fd, downloads[downloads_amt-1].bytes_downloaded);

                    // printf("%s\n", strtok(downloads[downloads_amt-1].url, "/"));
                    // printf("%s\n", strtok(NULL, "/")); // hostname

                    // prepare connection
                    connect_addr.sin_family = AF_INET;
                    connect_addr.sin_port = htons(PORT);
                    int tmp = inet_pton(AF_INET, "127.0.0.1", &connect_addr.sin_addr);

                    printf("%d\n", tmp);

                    // connect to server socket
                    tmp = connect(downloads[downloads_amt-1].socket_fd, (struct sockaddr*)&connect_addr, sizeof(connect_addr));

                    printf("%d\n", tmp);

                    // send download request
                    tmp = send(downloads[downloads_amt-1].socket_fd, "GET /downloads/test.txt HTTP/1.0\r\nHost: localhost\r\n\r\n\r\n", 55, 0);

                    printf("%d\n", tmp);
                }

                // handle stop TODO
                else {
                    printf("stopping\n");
                }

            }

        }

        // handle server
        if(fds[1].revents & POLLIN) {

            // allocate space for new upload
            uploads = realloc(uploads, sizeof(struct upload) * ++uploads_amt);
            uploads_poll_fds = realloc(uploads_poll_fds, sizeof(struct pollfd) * uploads_amt);

            // populate new upload with socket fd
            uploads[uploads_amt-1].socket_fd = accept(fds[1].fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);

            // populate upload fd
            uploads_poll_fds[uploads_amt-1].fd = uploads[uploads_amt-1].socket_fd;
            uploads_poll_fds[uploads_amt-1].events = POLLOUT;

            // get uri
            read(uploads[uploads_amt-1].socket_fd, buf, BUFFER);
            strtok(buf, " ");
            token = strtok(NULL, " ");
            token += 1;

            // populate new upload with file fd
            uploads[uploads_amt-1].file_fd = open(token, O_RDONLY);
        }

        // poll upload fds
        poll(uploads_poll_fds, uploads_amt, poll_timeout_ms);

        // handle uploads
        for (int i = 0; i < uploads_amt; i++) {

            // TODO stop (POLLHUP)

            if(uploads_poll_fds[i].revents & POLLOUT) {

                // read single byte
                bytes_read = read(uploads[i].file_fd, &byte, 1);

                // if byte read - send
                if (bytes_read) {
                    printf("writing byte\n");
                    send(uploads[i].socket_fd, &byte, 1, 0);
                    printf("wrote byte\n");
                }

                // else eof - close fds
                else {
                    close(uploads[i].file_fd);
                    close(uploads[i].socket_fd);

                    // remove upload from uploads
                    for(int j = 0; j < uploads_amt; j++) {
                        uploads[j] = uploads[j+1];
                        uploads_poll_fds[j] = uploads_poll_fds[j+1];
                    }
                    uploads = realloc(uploads, sizeof(struct upload) * --uploads_amt);
                    uploads_poll_fds = realloc(uploads_poll_fds, sizeof(struct pollfd) * uploads_amt);
                    i--;

                    printf("removed from uploads array...\n");
                }
        }

        }

        // poll download fds
        poll(downloads_poll_fds, downloads_amt, poll_timeout_ms);

        // handle downloads
        for (int i = 0; i < downloads_amt; i++) {

            // if received input from socket
            if (downloads_poll_fds[i].revents & POLLIN) {

                printf("reading byte\n");

                // read from socket
                bytes_read = recv(downloads[i].socket_fd, &byte, 1, 0);

                printf("read %d bytes\n", bytes_read);

                // 0 means EOF (which still triggers POLLIN) - close the socket
                if (bytes_read == 0) {
                    close(downloads[i].file_fd);
                    close(downloads[i].socket_fd);

                    // remove download from downloads
                    for(int j = 0; j < downloads_amt; j++) {
                        downloads[j] = downloads[j+1];
                        downloads_poll_fds[j] = downloads_poll_fds[j+1];
                    }
                    downloads = realloc(downloads, sizeof(struct download) * --downloads_amt);
                    downloads_poll_fds = realloc(downloads_poll_fds, sizeof(struct pollfd) * downloads_amt);
                    i--;

                    printf("removed from downloads array...\n");
                }
                
                // write byte to file
                else {
                    write(downloads[i].file_fd, &byte, 1);
                    downloads[i].bytes_downloaded++;
                }
            }

        }

    }

    // release memory
    free(downloads);
    free(downloads_poll_fds);
    free(uploads);
    free(uploads_poll_fds);

    // TODO close server properly

    close(fds[1].fd);

    printf("bye\n");

}