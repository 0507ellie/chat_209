#ifndef CLIENT_MGR_H
#define CLIENT_MGR_H

#include "protocol.h"

/* All possible states a connected client can be in. */
typedef enum {
    CLIENT_HANDSHAKING, /* connected but hasn't sent MSG_CONNECT yet */
    CLIENT_ACTIVE,      /* fully registered with a username          */
} ClientState;

/* Everything the server tracks about one connected client. */
typedef struct {
    int         fd;                      /* socket file descriptor          */
    ClientState state;                   /* handshaking or active           */
    char        username[MAX_USERNAME];  /* set on MSG_CONNECT              */
    char        channel[MAX_CHANNEL];    /* current channel, "" if none     */
    uint32_t    last_msg_id;             /* last message ID seen by client  */
} Client;

/*
 * The global client table.
 * Defined in client_mgr.c, declared here so other modules can iterate it.
 * Use the functions below instead of touching this directly where possible.
 */
extern Client  clients[MAX_CLIENTS];
extern int     client_count;

/*
 * Register a new connection. Call this right after accept().
 * Returns a pointer to the new Client slot, or NULL if the table is full.
 */
Client *client_add(int fd);

/*
 * Remove a client by file descriptor. Closes the socket.
 * Safe to call even if fd is not in the table.
 */
void client_remove(int fd);

/*
 * Look up a client by file descriptor.
 * Returns NULL if not found.
 */
Client *client_find_by_fd(int fd);

/*
 * Look up an active client by username (case-sensitive).
 * Returns NULL if not found or if the client is still handshaking.
 */
Client *client_find_by_name(const char *username);

/*
 * Returns 1 if the username is already taken by an active client, 0 otherwise.
 */
int client_username_taken(const char *username);

#endif /* CLIENT_MGR_H */