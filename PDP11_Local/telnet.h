#ifndef TELNET_H
#define TELNET_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_CLIENTS 8

typedef struct {
    int socket;
    bool connected;
    uint8_t buffer[1024];
    int buffer_len;
} client_t;

typedef struct {
    int server_socket;
    int port;
    client_t clients[MAX_CLIENTS];
    int num_clients;
} telnet_server_t;

/* Function prototypes */
telnet_server_t* telnet_init(int port);
void telnet_free(telnet_server_t *server);
void telnet_accept_connections(telnet_server_t *server);
bool telnet_has_input(telnet_server_t *server);
uint8_t telnet_read_byte(telnet_server_t *server);
void telnet_write_byte(telnet_server_t *server, uint8_t byte);
void telnet_write(telnet_server_t *server, const uint8_t *data, int len);

#endif // TELNET_H
