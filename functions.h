
struct client;

typedef enum { false, true } bool;

int is_private_message(char *buffer);

void find_private_message_receiver(char *receiver, char *buffer, int max_sd, struct client *client_socket);

void send_private_message(char *buffer, int max_sd, struct client *client_socket, fd_set *read_fd, char *sender);

void set_socket_options(struct sockaddr_in *address);

void add_listener_set(int socket_fd, fd_set *read_fd);

void add_connection_sets(struct client *client_socket, fd_set *read_fd, int *max_sd);

void add_client_connection(int listener, fd_set *read_fs, int *max_sd, struct sockaddr_in address, int addrlen, struct client *client_socket);

void add_client_username(int listener, fd_set *read_fs, int *max_sd, struct client *client_socket, int new_socket);

void read_in_message(int sock_desc, char *buffer);

void broadcast_new_message(int sock_desc, char *buffer, int max_sd, struct client *client_socket, fd_set *read_fd);

void read_incoming_message(char *buffer, int max_sd, struct client *client_socket, fd_set *read_fd);

int wait_for_input(int listener, fd_set *read_fd, int *max_sd, struct client *client_socket);

void append_username_to_message(char *buffer, char *name);
