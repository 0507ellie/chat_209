#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <sys/socket.h>

#include "protocol.h"
#include "socket.h"
#include "display.h"
#include "input.h"

static char my_username[MAX_USERNAME];    // user's username
static char current_channel[MAX_CHANNEL]; // channel the user is currently in

/*
  Send a Message (struct) over a socket, retrying until the full message (all bytes) has been successfully transmitted.
  Print an error if send() fails, and return early.
*/
static void send_msg(int fd, const Message *m)
{
  const char *buf = (const char *)m;
  size_t total = sizeof(*m); // total number of bytes in message we're sending
  size_t sent = 0;           // number of bytes sent so far
  while (sent < total)
  {
    ssize_t n = send(fd, buf + sent, total - sent, 0); // contains number of bytes successfully sent and -1 on error
    if (n < 0)
    {
      perror("send");
      return;
    }
    sent += (size_t)n;
  }
}

/*
  Update current_channel when the user joins or leaves a channel.
*/
static void update_channel(const Message *m)
{
  if (m->type == MSG_JOINED && strcmp(m->sender, my_username) == 0) // handle joining a channel
  {
    strncpy(current_channel, m->target, MAX_CHANNEL - 1);
  }
  else if (m->type == MSG_LEFT && strcmp(m->sender, my_username) == 0) // handle leaving a channel
  {
    current_channel[0] = '\0';
  }
}

int main(int argc, char *argv[])
{
  // default to localhost unless a server address was passed as a command-line argument
  const char *server_host = "127.0.0.1";
  if (argc >= 2)
  {
    server_host = argv[1];
  }

  // prompt user for a username and handle error-checking (fgets failing, empty username)
  printf("Username:\n");
  if (!fgets(my_username, sizeof(my_username), stdin))
  {
    return 1;
  }
  my_username[strcspn(my_username, "\n")] = '\0'; // replaces "\n" with a null terminator; safer than overwriting last character
  if (my_username[0] == '\0')
  {
    fprintf(stderr, "Username cannot be empty.\n");
    return 1;
  }

  int fd = connect_to_server(SERVER_PORT, server_host);

  // register the user's username
  Message m = {0};
  m.type = MSG_CONNECT;
  strncpy(m.sender, my_username, MAX_USERNAME - 1);
  send_msg(fd, &m);

  // notify to user that program is attempting connection
  printf("Connecting as '%s'...  (type /help for commands)\n", my_username);

  // register two fds with poll(), one for user input (stdin) and one for incoming server messages (socket)
  struct pollfd fds[2];
  fds[0].fd = STDIN_FILENO;
  fds[0].events = POLLIN;
  fds[1].fd = fd;
  fds[1].events = POLLIN;

  while (1)
  {
    // wait for both fds and handle error-checking
    int ready = poll(fds, 2, -1);
    if (ready < 0)
    {
      perror("poll");
      break;
    }

    // handle user input (user typed something) accordingly; see display.c for valid input
    if (fds[0].revents & POLLIN)
    {
      printf("> ");
      fflush(stdout);
      char line[MAX_BODY];
      if (!fgets(line, sizeof(line), stdin))
      {
        m = (Message){0};
        m.type = MSG_DISCONNECT;
        strncpy(m.sender, my_username, MAX_USERNAME - 1);
        send_msg(fd, &m);
        break;
      }

      Message out = {0};
      int result = input_parse(my_username, current_channel, line, &out);

      if (result == -2)
        display_help();
      else if (result == 0)
        send_msg(fd, &out);
      else if (result == 1)
      {
        send_msg(fd, &out);
        break;
      }
    }

    // handle if connection is unexpectedly lost
    if (fds[1].revents & (POLLHUP | POLLERR))
    {
      printf("Connection lost.\n");
      break;
    }

    // handle incoming messages from server
    if (fds[1].revents & POLLIN)
    {
      Message incoming = {0};
      ssize_t n = recv(fd, &incoming, sizeof(incoming), MSG_WAITALL);
      if (n <= 0)
      {
        printf("Server disconnected.\n");
        break;
      }
      update_channel(&incoming);
      display_message(&incoming, my_username);
      if (incoming.type == MSG_CONNECT_ERR)
        break;
    }
  }

  close(fd);
  return 0;
}
