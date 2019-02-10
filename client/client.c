
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT "1776"
#define MAXDATASIZE 100
#define FAMILY AF_INET
#define SOCKTYPE SOCK_STREAM

// Clear out read file descriptor set then add socket/stdin to read fds
void setup_fd_sets(int socket_fd, fd_set *read_fd)
{
        FD_ZERO(read_fd);
        FD_SET(STDIN_FILENO, read_fd);
        FD_SET(socket_fd, read_fd);

}


void receive_message(int socket_fd, char *in_buf)
{
    int msg_len;

    // Empty buffer and store incoming message
    memset(in_buf, 0, (sizeof in_buf));
    if((msg_len = recv(socket_fd, in_buf, MAXDATASIZE, 0)) == -1)
    {
        perror("recv: ");
    }
    // End message with terminating char
    in_buf[msg_len] = '\0';
}

void print_message(char *message)
{
    printf("== Server Reply == \n%s\n", message);
}

void handle_stdin(char *out_buf)
{
    // Read in message
    int msg_len;
    if((msg_len = read(STDIN_FILENO, out_buf, MAXDATASIZE)) == -1)
    {
        perror("read: dis");
    }
    // Place terminating char at end
    if (msg_len > 0 && out_buf[msg_len - 1] == '\n')
        out_buf[msg_len - 1] = '\0';
}

void set_hint_options(struct addrinfo *hints)
{
    hints->ai_family = FAMILY;
    hints->ai_socktype = SOCKTYPE;
}

int connect_to_server(struct addrinfo *serv_info, struct addrinfo *temp, int *socket_fd)
{
    for(temp = serv_info; temp != NULL; temp = temp->ai_next)
    {
        if ((*socket_fd = socket(temp->ai_family, temp->ai_socktype,
                temp->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(*socket_fd, temp->ai_addr, temp->ai_addrlen) == -1) {
            close(*socket_fd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (temp == NULL)
    {
        fprintf(stderr, "client: failed to connect\n");
        exit(0);
    }
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
    int socket_fd, numbytes;
    char in_buf[MAXDATASIZE], out_buf[MAXDATASIZE];
    struct addrinfo hints, *serv_info;
    char server_name[INET6_ADDRSTRLEN];

    fd_set read_fd;

    if (argc != 2) {
        fprintf(stderr,"Enter hostname as argument.\n");
        exit(0);
    }

    // Clear out hints and set for IPv4, TCP
    memset(&hints, 0, sizeof hints);
    set_hint_options(&hints);

    // store server info in serv_info
    getaddrinfo(argv[1], PORT, &hints, &serv_info);

    struct addrinfo *temp;
    connect_to_server(serv_info, temp, &socket_fd);

    // inet_ntop(temp->ai_family, get_in_addr((struct sockaddr *)temp->ai_addr), server_name, sizeof server_name);
    // printf("client: connecting to %server_name\n", server_name);
    printf("Connecting...\n");

    // Free up structure
    freeaddrinfo(serv_info);

    // Run continuously
    while (1)
    {
        setup_fd_sets(socket_fd, &read_fd);

        // Wait for activity indefinitely
        // Only one socket, so max fd is socket_fd
        if(select(socket_fd + 1, &read_fd, NULL, NULL, NULL) == -1)
        {
            perror("select(): ");
            printf("blaasd");

            exit(0);
        }

        // If triggered by incoming data from server
        if(FD_ISSET(socket_fd, &read_fd))
        {
            receive_message(socket_fd, in_buf);
            print_message(in_buf);
        }

        // If triggered by user input
        if(FD_ISSET(STDIN_FILENO, &read_fd))
        {
            // prepare send buffer
            handle_stdin(out_buf);

            // send message to server
            send(socket_fd, out_buf, strlen(out_buf), 0);

            // clear out stin buffer
            fflush(STDIN_FILENO);
        }

    }


    return 0;
}
