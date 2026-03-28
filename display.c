#include <stdio.h>
#include <string.h>
#include <time.h>

#include "protocol.h"
#include "display.h"

/* ANSI color codes */
#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define DIM     "\033[2m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define CYAN    "\033[36m"

/*
  Null-terminate a string with a trailing newline.
*/
static void null_terminate(char *s)
{
  size_t len = strlen(s);
  if (len > 0 && s[len - 1] == '\n')
    s[len - 1] = '\0';
}

static void print_timestamp(void)
{
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  printf(DIM "[%02d:%02d] " RESET, t->tm_hour, t->tm_min);
}

static void print_divider(void)
{
  printf(DIM "──────────────────────────────────────\n" RESET);
}

/*
  Display messages upon action (connecting to server, private messaging, joining channel, leaving channel,
  checking online status).
*/
void display_message(const Message *m, const char *my_username)
{
  Message msg = *m;
  null_terminate(msg.body);
  null_terminate(msg.sender);
  null_terminate(msg.target);

  switch (msg.type)
  {
  case MSG_CONNECT_OK:
    printf("\n");
    printf(CYAN "░█████╗░██╗░░██╗░█████╗░████████╗\n" RESET);
    printf(CYAN "██╔══██╗██║░░██║██╔══██╗╚══██╔══╝\n" RESET);
    printf(CYAN "██║░░╚═╝███████║███████║░░░██║░░░\n" RESET);
    printf(CYAN "██║░░██╗██╔══██║██╔══██║░░░██║░░░\n" RESET);
    printf(CYAN "╚█████╔╝██║░░██║██║░░██║░░░██║░░░\n" RESET);
    printf(CYAN "░╚════╝░╚═╝░░╚═╝╚═╝░░╚═╝░░░╚═╝░░░\n" RESET);
    printf("\n");
    printf(GREEN "  %s\n" RESET, msg.body);
    print_divider();
    printf("\n");
    break;

  case MSG_CONNECT_ERR:
    printf(RED "[error] Could not connect: %s\n" RESET, msg.body);
    break;

  case MSG_CHAT_RECV:
    print_timestamp();
    printf("[#%s] " BOLD "%s" RESET ": %s\n", msg.target, msg.sender, msg.body);
    break;

  case MSG_PRIVATE_RECV:
    print_timestamp();
    if (strcmp(msg.sender, my_username) == 0)
      printf(CYAN "[DM to %s] %s\n" RESET, msg.target, msg.body);
    else
      printf(CYAN "[DM from %s] %s\n" RESET, msg.sender, msg.body);
    break;

  case MSG_JOINED:
    print_divider();
    printf(DIM "  --> %s joined #%s\n" RESET, msg.sender, msg.target);
    print_divider();
    break;

  case MSG_LEFT:
    print_divider();
    printf(DIM "  <-- %s left #%s\n" RESET, msg.sender, msg.target);
    print_divider();
    break;

  case MSG_ONLINE_RESP:
    printf(YELLOW "[online] %s\n" RESET, msg.body);
    break;

  case MSG_ERROR:
    printf(RED "[!] %s\n" RESET, msg.body);
    break;

  default:
    break;
  }

  fflush(stdout);
}

/*
  Print help commands.
*/
void display_help(void)
{
  printf("\n" BOLD "Available commands:\n" RESET);
  printf("  /join <channel>       join a channel\n");
  printf("  /leave                leave current channel\n");
  printf("  /msg <user> <text>    send a private message\n");
  printf("  /online               list online users\n");
  printf("  /help                 show this message\n");
  printf("  /quit                 disconnect and exit\n");
  printf("\n");
  fflush(stdout);
}
