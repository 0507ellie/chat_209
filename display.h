#ifndef DISPLAY_H
#define DISPLAY_H

#include "protocol.h"

/*
  Display messages upon action (connecting to server, private messaging, joining channel, leaving channel,
  checking online status).
*/
void display_message(const Message *m, const char *my_username);

/*
  Print help commands.
*/
void display_help(void);

#endif /* DISPLAY_H */