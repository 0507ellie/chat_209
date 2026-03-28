#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "socket.h"
#include "protocol.h"
#include "client_manager.h"

#ifndef PORT
#define PORT SERVER_PORT
#endif

static uint32_t next_msg_id = 1;

/*Helper functions*/

/* Send one Message to a single file descriptor. Returns -1 on error. */
static int send_to_client(int fd, Message *msg) {
    ssize_t n = write(fd, msg, sizeof(Message));
    if (n != (ssize_t)sizeof(Message)) {
        perror("write");
        return -1;
    }
    return 0;
}

/*Broadcast msg to every active client in the given channel. if exclude_fd >= 0, that fd is skipped.*/
static void broadcast_channel(const char *channel, Message *msg, int exclude_fd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd <= 0) continue;
        if (clients[i].state != CLIENT_ACTIVE) continue;
        if (strcmp(clients[i].channel, channel) != 0) continue;
        if (clients[i].fd == exclude_fd) continue;
        send_to_client(clients[i].fd, msg);
    }
}

/* Set up the listening socket using the same helpers as the select-demo. */
static int bindandlisten(void) {
    struct sockaddr_in *self = init_server_addr(PORT);
    int fd = set_up_server_socket(self, 10);
    free(self);
    return fd;
}

/* Message handlers */

static void handle_connect(Client *c, Message *msg) {
    if (c->state == CLIENT_ACTIVE) return;  /* ignore duplicate MSG_CONNECT */

    if (msg->sender[0] == '\0') {
        Message err;
        memset(&err, 0, sizeof(err));
        err.type = MSG_CONNECT_ERR;
        strncpy(err.body, "Invalid username", MAX_BODY - 1);
        send_to_client(c->fd, &err);
        return;
    }

    if (client_username_taken(msg->sender)) {
        Message err;
        memset(&err, 0, sizeof(err));
        err.type = MSG_CONNECT_ERR;
        strncpy(err.body, "Username already taken", MAX_BODY - 1);
        send_to_client(c->fd, &err);
        return;
    }

    strncpy(c->username, msg->sender, MAX_USERNAME - 1);
    c->state = CLIENT_ACTIVE;

    Message ok;
    memset(&ok, 0, sizeof(ok));
    ok.type = MSG_CONNECT_OK;
    strncpy(ok.body, "Welcome!", MAX_BODY - 1);
    send_to_client(c->fd, &ok);

    printf("[+] %s connected\n", c->username);
}

static void handle_join(Client *c, Message *msg) {
    if (c->state != CLIENT_ACTIVE) return;

    /* Auto-leave current channel first */
    if (c->channel[0] != '\0') {
        Message left;
        memset(&left, 0, sizeof(left));
        left.type   = MSG_LEFT;
        left.msg_id = next_msg_id++;
        strncpy(left.sender, c->username, MAX_USERNAME - 1);
        strncpy(left.target, c->channel,  MAX_CHANNEL - 1);
        broadcast_channel(c->channel, &left, c->fd);
    }

    strncpy(c->channel, msg->target, MAX_CHANNEL - 1);

    Message joined;
    memset(&joined, 0, sizeof(joined));
    joined.type   = MSG_JOINED;
    joined.msg_id = next_msg_id++;
    strncpy(joined.sender, c->username, MAX_USERNAME - 1);
    strncpy(joined.target, c->channel,  MAX_CHANNEL - 1);

    broadcast_channel(c->channel, &joined, c->fd);
    send_to_client(c->fd, &joined); /* confirm to the joiner */

    printf("[~] %s joined #%s\n", c->username, c->channel);
}

static void handle_leave(Client *c) {
    if (c->state != CLIENT_ACTIVE || c->channel[0] == '\0') return;

    Message left;
    memset(&left, 0, sizeof(left));
    left.type   = MSG_LEFT;
    left.msg_id = next_msg_id++;
    strncpy(left.sender, c->username, MAX_USERNAME - 1);
    strncpy(left.target, c->channel,  MAX_CHANNEL - 1);

    broadcast_channel(c->channel, &left, c->fd);
    send_to_client(c->fd, &left); /* confirm to the leaver */

    printf("[~] %s left #%s\n", c->username, c->channel);
    c->channel[0] = '\0';
}

static void handle_chat(Client *c, Message *msg) {
    if (c->state != CLIENT_ACTIVE) return;

    if (c->channel[0] == '\0') {
        Message err;
        memset(&err, 0, sizeof(err));
        err.type = MSG_ERROR;
        strncpy(err.body, "You are not in a channel", MAX_BODY - 1);
        send_to_client(c->fd, &err);
        return;
    }

    Message out;
    memset(&out, 0, sizeof(out));
    out.type   = MSG_CHAT_RECV;
    out.msg_id = next_msg_id++;
    strncpy(out.sender, c->username, MAX_USERNAME - 1);
    strncpy(out.target, c->channel,  MAX_CHANNEL - 1);
    strncpy(out.body,   msg->body,   MAX_BODY - 1);

    broadcast_channel(c->channel, &out, -1); /* include sender */
}

static void handle_private(Client *c, Message *msg) {
    if (c->state != CLIENT_ACTIVE) return;

    Client *target = client_find_by_name(msg->target);
    if (target == NULL) {
        Message err;
        memset(&err, 0, sizeof(err));
        err.type = MSG_ERROR;
        snprintf(err.body, MAX_BODY, "User '%s' not found", msg->target);
        send_to_client(c->fd, &err);
        return;
    }

    Message out;
    memset(&out, 0, sizeof(out));
    out.type   = MSG_PRIVATE_RECV;
    out.msg_id = next_msg_id++;
    strncpy(out.sender, c->username, MAX_USERNAME - 1);
    strncpy(out.target, msg->target, MAX_CHANNEL - 1);
    strncpy(out.body,   msg->body,   MAX_BODY - 1);

    send_to_client(target->fd, &out);
    send_to_client(c->fd, &out); /* echo to sender */
}

static void handle_online(Client *c) {
    if (c->state != CLIENT_ACTIVE) return;

    char list[MAX_BODY];
    list[0] = '\0';
    int first = 1;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd <= 0) continue;
        if (clients[i].state != CLIENT_ACTIVE) continue;
        if (!first) strncat(list, ",", MAX_BODY - strlen(list) - 1);
        strncat(list, clients[i].username, MAX_BODY - strlen(list) - 1);
        first = 0;
    }

    Message resp;
    memset(&resp, 0, sizeof(resp));
    resp.type = MSG_ONLINE_RESP;
    strncpy(resp.body, list, MAX_BODY - 1);
    send_to_client(c->fd, &resp);
}

static void handle_disconnect(Client *c) {
    if (c->state == CLIENT_ACTIVE) {
        printf("[-] %s disconnected\n", c->username);
        if (c->channel[0] != '\0') {
            Message left;
            memset(&left, 0, sizeof(left));
            left.type   = MSG_LEFT;
            left.msg_id = next_msg_id++;
            strncpy(left.sender, c->username, MAX_USERNAME - 1);
            strncpy(left.target, c->channel,  MAX_CHANNEL - 1);
            broadcast_channel(c->channel, &left, c->fd);
        }
    } else {
        printf("[-] unnamed client (fd %d) disconnected\n", c->fd);
    }
    client_remove(c->fd); /* closes the socket */
}

/* Dispatch a received message to the right handler. */
static void handle_message(Client *c, Message *msg) {
    switch (msg->type) {
        case MSG_CONNECT:    handle_connect(c, msg);  break;
        case MSG_JOIN:       handle_join(c, msg);     break;
        case MSG_LEAVE:      handle_leave(c);         break;
        case MSG_CHAT:       handle_chat(c, msg);     break;
        case MSG_PRIVATE:    handle_private(c, msg);  break;
        case MSG_ONLINE:     handle_online(c);        break;
        case MSG_DISCONNECT: handle_disconnect(c);    break;
        default:
            fprintf(stderr, "Unknown message type %d from fd %d\n",
                    msg->type, c->fd);
            break;
    }
}

/* Main */

int main(void) {
    int listenfd = bindandlisten();
    printf("Server listening on port %d\n", PORT);

    fd_set allset, rset;
    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);
    int maxfd = listenfd;

    while (1) {
        rset = allset; /* reset before each select call */

        struct timeval tv;
        tv.tv_sec  = 10;
        tv.tv_usec = 0;

        int nready = select(maxfd + 1, &rset, NULL, NULL, &tv);
        if (nready == 0) {
            printf("No activity in %ld seconds\n", tv.tv_sec);
            continue;
        }
        if (nready == -1) {
            perror("select");
            continue;
        }

        /* New incoming connection */
        if (FD_ISSET(listenfd, &rset)) {
            int clientfd = accept_connection(listenfd);
            if (clientfd < 0) {
                exit(1);
            }

            FD_SET(clientfd, &allset);
            if (clientfd > maxfd) maxfd = clientfd;

            Client *c = client_add(clientfd);
            if (c == NULL) {
                fprintf(stderr, "Client table full, rejecting fd %d\n", clientfd);
                close(clientfd);
                FD_CLR(clientfd, &allset);
            }
        }

        /* Check each client fd for incoming data */
        for (int i = 0; i <= maxfd; i++) {
            if (!FD_ISSET(i, &rset)) continue;
            if (i == listenfd) continue;

            Client *c = client_find_by_fd(i);
            if (c == NULL) {
                FD_CLR(i, &allset);
                close(i);
                continue;
            }

            Message msg;
            int bytes_read = read(i, &msg, sizeof(Message));

            if (bytes_read > 0) {
                if (bytes_read == (int)sizeof(Message)) {
                    int tmp_fd = i;
                    handle_message(c, &msg);
                    /* If MSG_DISCONNECT was handled, client is gone */
                    if (client_find_by_fd(tmp_fd) == NULL) {
                        FD_CLR(tmp_fd, &allset);
                        if (tmp_fd == maxfd) {
                            while (maxfd > listenfd && !FD_ISSET(maxfd, &allset))
                                maxfd--;
                        }
                    }
                } else {
                    fprintf(stderr, "Partial read from fd %d (%d bytes), dropping\n",
                            i, bytes_read);
                }
            } else if (bytes_read == 0) {
                /* Client closed connection without MSG_DISCONNECT */
                printf("[-] fd %d hung up\n", i);
                int tmp_fd = i;
                handle_disconnect(c);
                FD_CLR(tmp_fd, &allset);
                if (tmp_fd == maxfd) {
                    while (maxfd > listenfd && !FD_ISSET(maxfd, &allset))
                        maxfd--;
                }
            } else {
                perror("read");
                int tmp_fd = i;
                handle_disconnect(c);
                FD_CLR(tmp_fd, &allset);
                if (tmp_fd == maxfd) {
                    while (maxfd > listenfd && !FD_ISSET(maxfd, &allset))
                        maxfd--;
                }
            }
        }
    }

    close(listenfd);
    return 0;
}
