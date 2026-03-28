CC     = cc
CFLAGS = -Wall -Wextra -g

all: client stub

client: client.o display.o input.o socket.o
	$(CC) $(CFLAGS) -o $@ $^

# Requires client_manager.c (not yet implemented)
server: server.o socket.o client_manager.o
	$(CC) $(CFLAGS) -o $@ $^

stub: stub.o
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o client server stub

.PHONY: all clean
