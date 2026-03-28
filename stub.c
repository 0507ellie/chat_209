#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "./protocol.h"

static void send_msg(int fd, MsgType type, const char *sender,
                     const char *target, const char *body)
{
  Message m = {0};
  m.type = type;
  if (sender)
    strncpy(m.sender, sender, MAX_USERNAME - 1);
  if (target)
    strncpy(m.target, target, MAX_CHANNEL - 1);
  if (body)
    strncpy(m.body, body, MAX_BODY - 1);
  send(fd, &m, sizeof(m), 0);
}

/*
  Stub server for testing client without actual server.
*/
int main(void)
{
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

  struct sockaddr_in addr = {
      .sin_family = AF_INET,
      .sin_addr.s_addr = INADDR_ANY,
      .sin_port = htons(SERVER_PORT),
  };
  bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
  listen(server_fd, 1);
  printf("Stub server listening on port %d...\n", SERVER_PORT);

  int fd = accept(server_fd, NULL, NULL);
  printf("Client connected.\n");

  // wait for MSG_CONNECT from the client
  Message incoming = {0};
  recv(fd, &incoming, sizeof(incoming), 0);
  printf("Client says their name is: %s\n", incoming.sender);

  // respond with welcome
  send_msg(fd, MSG_CONNECT_OK, NULL, NULL, "Welcome to the stub server!");
  sleep(1);

  // simulate activity
  send_msg(fd, MSG_JOINED, "alice", "general", "alice joined #general");
  sleep(1);
  send_msg(fd, MSG_CHAT_RECV, "alice", "general", "hey, anyone here?");
  sleep(1);
  send_msg(fd, MSG_CHAT_RECV, "bob", "general", "yep, just arrived");
  sleep(2);
  send_msg(fd, MSG_PRIVATE_RECV, "alice", NULL, "psst — check #random");
  sleep(1);
  send_msg(fd, MSG_ONLINE_RESP, NULL, NULL, "alice, bob, carol");
  sleep(1);
  send_msg(fd, MSG_ERROR, NULL, NULL, "example error message");
  sleep(1);
  send_msg(fd, MSG_LEFT, "bob", "general", "bob left #general");

  printf("Done sending test messages. Keeping connection open.\n");
  sleep(60);

  close(fd);
  close(server_fd);
  return 0;
}
