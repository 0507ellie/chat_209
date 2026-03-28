#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include "client_manager.h"

Client clients[MAX_CLIENTS];
int    client_count = 0;

Client *client_add(int fd){
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd <= 0) {
            memset(&clients[i], 0, sizeof(Client)); 
            clients[i].fd = fd;
            clients[i].state = CLIENT_HANDSHAKING;
            client_count++;
            return &clients[i];
        }
    }
    return NULL; 
}

void client_remove(int fd){
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd == fd) {
           close(fd);
           memset(&clients[i], 0, sizeof(Client));
           client_count--;
           return;
        }
    }
}

Client *client_find_by_fd(int fd){
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd == fd) {
           return &clients[i];
        }
    }
    return NULL;
}

Client *client_find_by_name(const char *username){
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd > 0 && clients[i].state == CLIENT_ACTIVE 
            && strcmp(clients[i].username, username) == 0 ) {
           return &clients[i];
        }
    }
    return NULL;
}

int client_username_taken(const char *username){
    if(client_find_by_name(username)!= NULL){
        return 1;
    }
    return 0;
}