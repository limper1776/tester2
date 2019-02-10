
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>

#include "../functions.h"

#define FAMILY AF_INET
#define ADDRESS INADDR_ANY
#define PORT 1776
#define MAXCONNECTIONS 10
#define BUFFERSIZE 1024
#define USERNAMESIZE 25



struct client
{
    int socket;
    char username[USERNAMESIZE];
    bool receive_private;
};

void set_socket_options(struct sockaddr_in *address)
{
    address->sin_family = FAMILY;
    address->sin_addr.s_addr = ADDRESS;
    address->sin_port = htons(PORT);
}

void add_listener_set(int socket_fd, fd_set *read_fd)
{
    //clear the socket set
    FD_ZERO(read_fd);

    //add master socket to set
    FD_SET(socket_fd, read_fd);
}

void add_connection_sets(struct client *client_socket, fd_set *read_fd, int *max_sd)
{
    // loop through all possible connections and find valid ones
    int i, sock_desc;
    for (i=0 ; i < MAXCONNECTIONS ; i++)
    {
        // current socket descriptor
        sock_desc = client_socket[i].socket;

        // add to read fd list if valid
        if(sock_desc > 0)
        {
            FD_SET(sock_desc, read_fd);
        }

        // keep track of highest fd
        if(sock_desc > *max_sd)
            *max_sd = sock_desc;
    }
}

void add_client_connection(int listener, fd_set *read_fs, int *max_sd, struct sockaddr_in address, int addrlen, struct client *client_socket)
{
    // accept new connection
    int new_socket;
    new_socket = accept(listener, (struct sockaddr *)&address, (socklen_t*)&addrlen);

    printf("adding client . . .\n");

    // add new client sockcet to first opening
    int i;
    for (i = 0; i < MAXCONNECTIONS; i++)
    {
        if( client_socket[i].socket == 0 )
        {
            client_socket[i].socket = new_socket;
            client_socket[i].receive_private = false;
            printf("Adding ...\n");
            break;
        }
    }

    add_client_username(listener, read_fs, max_sd, client_socket, new_socket);
}

void add_client_username(int listener, fd_set *read_fs, int *max_sd, struct client *client_socket, int new_socket)
{
    //send new connection greeting message and ask for username
    char *welcome_message = "Welcome! Please enter a username: ";
    send(new_socket, welcome_message, strlen(welcome_message), 0);
    puts("Welcome message sent successfully");

    wait_for_input(listener, read_fs, max_sd, client_socket);

    if (!FD_ISSET(listener, read_fs))
    {
        int sock_desc;
        char buf[USERNAMESIZE];
        // Incoming transmission
        // Loop through all connections
        int i;
        for (i=0; i < MAXCONNECTIONS; i++)
        {
            sock_desc = client_socket[i].socket;
            // if a connected client triggered read
            if (FD_ISSET(sock_desc, read_fs))
            {
                // read username and associate it with client
                read_in_message(sock_desc, buf);
                strcpy(client_socket[i].username, buf);
            }
        }
        char *username_message = "Username added successfully! Ready to receive messages!";
        send(new_socket, username_message, strlen(username_message), 0);
    }
}

void read_in_message(int sock_desc, char *buffer)
{
    int read_len;
    read_len = read(sock_desc, buffer, BUFFERSIZE);
    // Add terminating character to end
    buffer[read_len] = '\0';

}

void broadcast_new_message(int sock_desc, char *buffer, int max_sd, struct client *client_socket, fd_set *read_fd)
{
    // broadcast message to all connections
    int i;
    for(i = 0; i < max_sd; i++)
        {
        sock_desc = client_socket[i].socket;
            // Dont send to sender
            if(!FD_ISSET(sock_desc, read_fd))
            {
                send(sock_desc , buffer , strlen(buffer) , 0 );
            }
        }
}

void append_username_to_message(char *buffer, char *name)
{
    // prepare to add ": " after name
    char *new_chars = ": ";

    // add username and new chars to beginning of buffer
    char *temp = strdup(buffer);
    strcpy(buffer, name);
    strcat(buffer, new_chars);
    strcat(buffer, temp);

    //free the memory
    // free(temp);
    // free(new_chars);
}

void append_username_to_private_message(char *buffer, char *sender)
{
    // prepare to add "@" before name and ": " after name
    char *new_chars_1 = "[";
    char *new_chars_2 = "]: ";
    // for removing '@[receiver name]'
    int first_delimiter;
    first_delimiter = strcspn(buffer, " ");

    // add username and new chars to beginning of buffer
    char *temp = strdup(buffer);
    strcpy(buffer, new_chars_1);
    strcat(buffer, sender);
    strcat(buffer, new_chars_2);
    // chop of @[receiver name] from buffer
    strcat(buffer, &temp[first_delimiter]);

    //free the memory
    // free(temp);
    // free(new_chars);
}

void read_incoming_message(char *buffer, int max_sd, struct client *client_socket, fd_set *read_fd)
{
    printf("read_incoming...");
    char receiver[USERNAMESIZE];
    char sender[USERNAMESIZE];
    int sock_desc;
    // Incoming transmission
    // Loop through all connections
    int i;
    for (i=0; i < MAXCONNECTIONS; i++)
    {
        sock_desc = client_socket[i].socket;
        // if a connected client triggered read
        if (FD_ISSET(sock_desc, read_fd))
        {
            read_in_message(sock_desc, buffer);
            if (is_private_message(buffer))
            {
                strcpy(sender, client_socket[i].username);
                find_private_message_receiver(receiver, buffer, max_sd, client_socket);
                send_private_message(buffer, max_sd, client_socket, read_fd, sender);
            }
            else
            {
                append_username_to_message(buffer, client_socket[i].username);
                broadcast_new_message(sock_desc, buffer, max_sd, client_socket, read_fd);
            }


        }
    }
}

int is_private_message(char *buffer)
{
    if (buffer[0] == '@')
        return true;
    else
        return false;
}

void find_private_message_receiver(char *receiver, char *buffer, int max_sd, struct client *client_socket)
{
    printf("\nfind_private...1:");
    // Find location of first space
    int first_delimiter;
    first_delimiter = strcspn(buffer, " ");

    // extract reciever name
    memcpy(receiver, &buffer[1], first_delimiter);
    receiver[first_delimiter-1] = '\0';
    printf("\nfind_private...2:|%s|", receiver);

    int i;
    for(i = 0; i < max_sd; i++)
    {
        if(strncmp(receiver, client_socket[i].username, strlen(receiver)) == 0)
            client_socket[i].receive_private = true;
    }
}

void send_private_message(char *buffer, int max_sd, struct client *client_socket, fd_set *read_fd, char *sender)
{
    // broadcast message to all connections
    printf("\nsend_private...1:\n");
    int sock_desc;
    char username[USERNAMESIZE];
    int i;
    for(i = 0; i < max_sd; i++)
        {
            sock_desc = client_socket[i].socket;
            strcpy(username, client_socket[i].username);

            if ((client_socket[i].receive_private == true))
            {
                printf("\nsend_private...2\n");
                // append_username_to_private_message(buffer, username);
                append_username_to_private_message(buffer, sender);
                send(sock_desc, buffer, strlen(buffer), 0);
                client_socket[i].receive_private = false;
            }
        }
    printf("\nsend_private...3:\n");
}


int wait_for_input(int listener, fd_set *read_fd, int *max_sd, struct client *client_socket)
{
        // prepare fd sets (just read in this case)
        add_listener_set(listener, read_fd);
        *max_sd = listener;
        add_connection_sets(client_socket, read_fd, max_sd);

        // Wait for activity indefinitely
        int activity;
        activity = select( *max_sd + 1 , read_fd , NULL , NULL , NULL);
        return activity;
}

int main(int argc , char *argv[])
{
    int listener;
    int addrlen, max_sd;
    struct sockaddr_in address;
    char buffer[BUFFERSIZE];
    struct client client_socket[MAXCONNECTIONS];
    // int client_socket[MAXCONNECTIONS];
    fd_set read_fd;

    //initialise all client_socket[] to 0 so not checked
    memset(client_socket, 0, sizeof client_socket);

    // create a listening socket
    listener = socket(AF_INET , SOCK_STREAM , 0);

    // listener options (set by macros at top of file)
    set_socket_options(&address);

    // bind listener to PORT in macro
    bind(listener, (struct sockaddr *)&address, sizeof(address));

    // start listening to no more than MAXCONNECTIONS connections
    listen(listener, MAXCONNECTIONS);

    //accept the incoming connection
    addrlen = sizeof(address);
    puts("running . . .");

    while(1)
    {
        printf("loop\n");
        // Wait for activity indefinitely
        wait_for_input(listener, &read_fd, &max_sd, client_socket);

        if (FD_ISSET(listener, &read_fd))
        {
            add_client_connection(listener, &read_fd, &max_sd, address, addrlen, client_socket);
        }

        // Iotherwise its a connected socket
        read_incoming_message(buffer, max_sd, client_socket, &read_fd);
    }

    return 0;
}
