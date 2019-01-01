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
  
#define FAMILY AF_INET
#define ADDRESS INADDR_ANY
#define PORT 1776
#define MAXCONNECTIONS 10
#define BUFFERSIZE 1024

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

void add_connection_sets(int *client_socket, fd_set *read_fd, int *max_sd)
{
    // loop through all possible connections and find valid ones
    int i, sock_desc;
    for (i=0 ; i < MAXCONNECTIONS ; i++) 
    {
        // current socket descriptor
        sock_desc = client_socket[i];
         
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

void add_client_connection(int listener, struct sockaddr_in address, int addrlen, int *client_socket)
{
    // accept new connection
    int new_socket;
    new_socket = accept(listener, (struct sockaddr *)&address, (socklen_t*)&addrlen);
  
    //inform user of socket number - used in send and receive commands[DELETE]
    printf("New connection , socket fd is %d , ip is : %s , port : %d \n" , new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

    //send new connection greeting message[DELETE]
    char *message = "Welcome!\r\n";
    send(new_socket, message, strlen(message), 0);
    puts("Welcome message sent successfully");
      
    // add new client sockcet to first opening
    int i;
    for (i = 0; i < MAXCONNECTIONS; i++) 
    {
        if( client_socket[i] == 0 )
        {
            client_socket[i] = new_socket;
            printf("Adding to list of sockets as %d\n" , i); //[DELETE]
            break;
        }
    }
}

void read_in_message(int sock_desc, char *buffer)
{
    int read_len;
    read_len = read(sock_desc, buffer, BUFFERSIZE);
    // Add terminating character to end
    buffer[read_len] = '\0';
}

void broadcast_new_message(int sock_desc, char *buffer, int max_sd, int *client_socket, fd_set *read_fd)
{
    // broadcast message to all connections
    int i;
    for(i = 0; i < max_sd; i++)
        {
        sock_desc = client_socket[i];
            // Dont send to sender
            if(!FD_ISSET(sock_desc, read_fd))
            {
                send(sock_desc , buffer , strlen(buffer) , 0 );
            }
        }
}

void read_incoming_message(char *buffer, int max_sd, int *client_socket, fd_set *read_fd)
{
    int sock_desc;
    // Incoming transmission
    // Loop through all connections
    int i;
    for (i=0; i < MAXCONNECTIONS; i++) 
    {
        sock_desc = client_socket[i];
        // if a connected client triggered read
        if (FD_ISSET(sock_desc, read_fd)) 
        {
            read_in_message(sock_desc, buffer);
            broadcast_new_message(sock_desc, buffer, max_sd, client_socket, read_fd);
        }
    }
}
 
int main(int argc , char *argv[])
{
    int opt = 1;
    int listener , addrlen , new_socket , client_socket[MAXCONNECTIONS] , activity, i , read_len , sock_desc;
    int max_sd;
    struct sockaddr_in address;
    char buffer[BUFFERSIZE];  //data buffer of 1k
    fd_set read_fd;
  
    //initialise all client_socket[] to 0 so not checked
    for (i = 0; i < MAXCONNECTIONS; i++) 
    {
        client_socket[i] = 0;
    }
      
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
        // prepare fd sets (just read in this case)
        add_listener_set(listener, &read_fd);
        max_sd = listener;
        add_connection_sets(client_socket, &read_fd, &max_sd);
        
        // Wait for activity indefinitely
        activity = select( max_sd + 1 , &read_fd , NULL , NULL , NULL);
          
        // Listener trigger means incoming connection
        if (FD_ISSET(listener, &read_fd)) 
        {
            add_client_connection(listener, address, addrlen, client_socket);
        }
          
        // Iotherwise its a connected socket
        read_incoming_message(buffer, max_sd, client_socket, &read_fd);
    }
      
    return 0;
}