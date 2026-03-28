#include <stdio.h>
#include <string.h>

#include "protocol.h"
#include "input.h"

/*
  Parse the input by advancing past the "prefix," the leading command and optional spaces.
  Return a pointer to the first non-space character after the prefix, or NULL if there's nothing there.
*/
static const char *skip_prefix(const char *line, size_t prefix_len)
{
  const char *rest = line + prefix_len;
  while (*rest == ' ')
  {
    rest++;
  }
  if (*rest == '\0')
  {
    return NULL;
  }
  return rest;
}

/*
  Parse the input into a Message (struct) ready to send to the server.
  Return 0 on success, -1 on parse error or unknown command, -2 if command is /help, and 1 if command is /quit.
*/
int input_parse(const char *my_username, const char *current_channel, const char *line, Message *out)
{
  memset(out, 0, sizeof(*out));
  strncpy(out->sender, my_username, MAX_USERNAME - 1);

  char buf[MAX_BODY];
  strncpy(buf, line, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';
  size_t len = strlen(buf);
  if (len > 0 && buf[len - 1] == '\n')
  {
    buf[--len] = '\0';
  }

  // handle /join
  if (strncmp(buf, "/join ", 6) == 0)
  {
    const char *ch = skip_prefix(buf, 6);
    if (!ch)
    {
      fprintf(stderr, "Parse error or unknown command.\n");
      return -1;
    }
    out->type = MSG_JOIN;
    strncpy(out->target, ch, MAX_CHANNEL - 1);
    return 0;
  }

  // handle /leave
  if (strcmp(buf, "/leave") == 0)
  {
    if (*current_channel == '\0')
    {
      fprintf(stderr, "You are not in a channel.\n");
      return -1;
    }
    out->type = MSG_LEAVE;
    strncpy(out->target, current_channel, MAX_CHANNEL - 1);
    return 0;
  }

  // handle /msg
  if (strncmp(buf, "/msg ", 5) == 0)
  {
    const char *rest = skip_prefix(buf, 5);
    if (!rest)
    {
      fprintf(stderr, "Parse error or unknown command.\n");
      return -1;
    }

    const char *space = strchr(rest, ' ');
    if (!space || *(space + 1) == '\0')
    {
      fprintf(stderr, "Parse error or unknown command.\n");
      return -1;
    }

    out->type = MSG_PRIVATE;
    size_t name_len = (size_t)(space - rest);
    if (name_len >= MAX_USERNAME)
      name_len = MAX_USERNAME - 1;
    strncpy(out->target, rest, name_len);
    strncpy(out->body, space + 1, MAX_BODY - 1);
    return 0;
  }

  // handle /online
  if (strcmp(buf, "/online") == 0)
  {
    out->type = MSG_ONLINE;
    return 0;
  }

  // handle /help
  if (strcmp(buf, "/help") == 0)
  {
    return -2;
  }

  // handle /quit
  if (strcmp(buf, "/quit") == 0)
  {
    out->type = MSG_DISCONNECT;
    return 1;
  }

  if (buf[0] == '/')
  {
    fprintf(stderr, "Parse error or unknown command.\n", buf);
    return -1;
  }

  // notify user to join channel before messaging
  if (*current_channel == '\0')
  {
    fprintf(stderr, "Join a channel first with /join <channel>\n");
    return -1;
  }

  out->type = MSG_CHAT;
  strncpy(out->target, current_channel, MAX_CHANNEL - 1);
  strncpy(out->body, buf, MAX_BODY - 1);
  return 0;
}
