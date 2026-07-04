BIN_NAME = wayfl
CC = gcc
LIBS = -lwayland-client
XDG_PROTOCOL_DIR = /usr/share/wayland-protocols/stable/xdg-shell

all: xdg-shell-client-protocol.h
	mkdir -p bin
	$(CC) -o bin/$(BIN_NAME) *.c $(LIBS)

xdg-shell-client-protocol.h:
	wayland-scanner client-header $(XDG_PROTOCOL_DIR)/xdg-shell.xml xdg-shell-client-protocol.h
	wayland-scanner private-code  $(XDG_PROTOCOL_DIR)/xdg-shell.xml xdg-shell-protocol.c

clean:
	rm -rf bin xdg-shell-client-protocol.h xdg-shell-protocol.c

run: all
	./bin/$(BIN_NAME)

.PHONY: all clean run