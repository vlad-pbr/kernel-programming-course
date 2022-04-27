// prog.c

#include <netinet/in.h>
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <poll.h>

#define BUFFER 4096
#define PORT 8080

struct download {
    int index;
    char* url;
    int bytes_downloaded;
};

void main(int argc, char *argv[]) {

    // define variables
    struct sockaddr_in address;
    int bytes_read;
    int addrlen = sizeof(address);
    char buf[BUFFER];
    struct pollfd fds[2];

    // set up polling for stdin
    fds[0].fd = 0;
    fds[0].events = POLLIN;

    // create new TCP socket
    fds[1].fd = socket(AF_INET, SOCK_STREAM, 0);
    printf("server socker fd: %d\n", fds[1].fd);

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
            printf("%s\n", buf);

            // TODO parse input

        }

        // handle server
        if(fds[1].revents & POLLIN) {

            // accept and read single request
            int new_socket = accept(fds[1].fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
            read(new_socket, buf, BUFFER);
            printf("%s\n", buf);
            close(new_socket);
            
        }

    }

    // TODO close server

}