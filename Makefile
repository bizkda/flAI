BIN_NAME = wayfl
all: 
	mkdir -p bin
	$(CC) -o bin/$(BIN_NAME) *.c