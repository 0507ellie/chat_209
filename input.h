#ifndef INPUT_H
#define INPUT_H

#include "protocol.h"

/*
  Parse the input into a Message (struct) ready to send to the server.
  Return 0 on success, -1 on parse error or unknown command, -2 if command is /help, and 1 if command is /quit.
*/
int input_parse(const char *my_username, const char *current_channel,
                const char *line, Message *out);

#endif /* INPUT_H */