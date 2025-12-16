#include "telnet.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>

/* Set socket to non-blocking mode */
static void set_nonblocking(int socket) {
    int flags = fcntl(socket, F_GETFL, 0);
    fcntl(socket, F_SETFL, flags | O_NONBLOCK);
}

/* Initialize telnet server */
telnet_server_t* telnet_init(int port) {
    telnet_server_t *server = calloc(1, sizeof(telnet_server_t));
    if (!server) return NULL;
    
    server->port = port;
    
    // Create server socket
    server->server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server->server_socket < 0) {
        perror("socket");
        free(server);
        return NULL;
    }
    
    // Set socket options
    int opt = 1;
    setsockopt(server->server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    set_nonblocking(server->server_socket);
    
    // Bind to port
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    if (bind(server->server_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server->server_socket);
        free(server);
        return NULL;
    }
    
    // Listen
    if (listen(server->server_socket, 5) < 0) {
        perror("listen");
        close(server->server_socket);
        free(server);
        return NULL;
    }
    
    printf("Telnet server listening on port %d\n", port);
    
    return server;
}

/* Free telnet server */
void telnet_free(telnet_server_t *server) {
    if (server) {
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (server->clients[i].connected) {
                close(server->clients[i].socket);
            }
        }
        close(server->server_socket);
        free(server);
    }
}

/* Accept new connections */
void telnet_accept_connections(telnet_server_t *server) {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    int client_socket = accept(server->server_socket, 
                               (struct sockaddr*)&client_addr, 
                               &addr_len);
    
    if (client_socket < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("accept");
        }
        return;
    }
    
    // Find free slot
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!server->clients[i].connected) {
            server->clients[i].socket = client_socket;
            server->clients[i].connected = true;
            server->clients[i].buffer_len = 0;
            set_nonblocking(client_socket);
            server->num_clients++;
            
            printf("Client connected (slot %d)\n", i);
            
            // Send welcome message
            const char *welcome = "\r\nPDP-11 Emulator\r\n\r\n";
            send(client_socket, welcome, strlen(welcome), 0);
            
            return;
        }
    }
    
    // No free slots
    const char *msg = "Server full\r\n";
    send(client_socket, msg, strlen(msg), 0);
    close(client_socket);
}

/* Check if any client has input */
bool telnet_has_input(telnet_server_t *server) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!server->clients[i].connected) continue;
        
        // Try to read from socket
        uint8_t buf[256];
        ssize_t n = recv(server->clients[i].socket, buf, sizeof(buf), 0);
        
        if (n > 0) {
            // Add to buffer, filtering telnet control sequences
            for (ssize_t j = 0; j < n; j++) {
                uint8_t byte = buf[j];
                
                // Skip telnet IAC sequences
                if (byte == 0xFF) {
                    // Skip IAC and next 2 bytes
                    j += 2;
                    if (j >= n) break;
                    continue;
                }
                
                // Add to client buffer
                if (server->clients[i].buffer_len < (int)sizeof(server->clients[i].buffer)) {
                    server->clients[i].buffer[server->clients[i].buffer_len++] = byte;
                }
            }
        } else if (n == 0 || (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
            // Client disconnected
            printf("Client disconnected (slot %d)\n", i);
            close(server->clients[i].socket);
            server->clients[i].connected = false;
            server->num_clients--;
        }
    }
    
    // Check if any client has buffered data
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server->clients[i].connected && server->clients[i].buffer_len > 0) {
            return true;
        }
    }
    
    return false;
}

/* Read a byte from first client with data */
uint8_t telnet_read_byte(telnet_server_t *server) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server->clients[i].connected && server->clients[i].buffer_len > 0) {
            uint8_t byte = server->clients[i].buffer[0];
            
            // Shift buffer
            memmove(server->clients[i].buffer, 
                   server->clients[i].buffer + 1, 
                   server->clients[i].buffer_len - 1);
            server->clients[i].buffer_len--;
            
            return byte;
        }
    }
    
    return 0;
}

/* Write byte to all connected clients */
void telnet_write_byte(telnet_server_t *server, uint8_t byte) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server->clients[i].connected) {
            send(server->clients[i].socket, &byte, 1, 0);
        }
    }
}

/* Write data to all connected clients */
void telnet_write(telnet_server_t *server, const uint8_t *data, int len) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server->clients[i].connected) {
            send(server->clients[i].socket, data, len, 0);
        }
    }
}
