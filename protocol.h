#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

#define MAX_USERNAME  32
#define MAX_CHANNEL   32
#define MAX_BODY      512
#define MAX_CLIENTS   64
#define MAX_CHANNELS  16
#define SERVER_PORT   8080

typedef enum {
    MSG_CONNECT,       /* client -> server: "I am username X"             */
    MSG_CONNECT_OK,    /* server -> client: "welcome, accepted"           */
    MSG_CONNECT_ERR,   /* server -> client: "username taken / invalid"    */

    MSG_JOIN,          /* client -> server: join channel in target        */
    MSG_LEAVE,         /* client -> server: leave channel in target       */
    MSG_JOINED,        /* server -> client: broadcast "X joined #channel" */
    MSG_LEFT,          /* server -> client: broadcast "X left #channel"  */

    MSG_CHAT,          /* client -> server: message to channel in target  */
    MSG_CHAT_RECV,     /* server -> client: deliver chat message          */

    MSG_PRIVATE,       /* client -> server: /msg <user> <text>            */
    MSG_PRIVATE_RECV,  /* server -> client: deliver private message       */

    MSG_ONLINE,        /* client -> server: /online request               */
    MSG_ONLINE_RESP,   /* server -> client: body = comma-separated names  */

    MSG_DISCONNECT,    /* client -> server: graceful quit                 */
    MSG_ERROR,         /* server -> client: generic error, reason in body */
} MsgType;

/*
 * The single wire format used for every message.
 * Both client and server always send and receive exactly sizeof(Message) bytes.
 *
 * Fields:
 *   type     - what kind of message this is (see MsgType above)
 *   msg_id   - unique ID assigned by the server
 *   sender   - username of the originating client (set by server)
 *   target   - channel name (e.g. "general") or recipient username
 *   body     - the text payload; meaning depends on type
 */
typedef struct {
    MsgType  type;
    uint32_t msg_id;
    char     sender[MAX_USERNAME];
    char     target[MAX_CHANNEL];
    char     body[MAX_BODY];
} Message;

#endif /* PROTOCOL_H */
