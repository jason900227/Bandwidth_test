# toolchain
ARM_CC = arm-linux-gnueabihf-gcc
CC = gcc

CFLAGS = -Wall -Wextra -g

# source files
SERVER_SRC = src/server.c
CLIENT_SRC = src/client.c

# output
BUILD_DIR = build

all: server client util

server:
	mkdir -p $(BUILD_DIR)
	$(ARM_CC) $(CFLAGS) $(SERVER_SRC) -o $(BUILD_DIR)/server

client:
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CLIENT_SRC) -o $(BUILD_DIR)/client

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all server client util clean