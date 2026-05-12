# toolchain
ARM_CC = arm-linux-gnueabihf-gcc
CC = gcc

# flags
CFLAGS = -Wall -Wextra -g

# directories
SRC_DIR = src
BUILD_DIR = build

# source files
SERVER_SRC = $(SRC_DIR)/server.c
CLIENT_SRC = $(SRC_DIR)/client.c

# targets
SERVER_BIN = $(BUILD_DIR)/server
CLIENT_BIN = $(BUILD_DIR)/client

all: server client

server: $(SERVER_BIN)

client: $(CLIENT_BIN)

$(SERVER_BIN): $(SERVER_SRC)
	mkdir -p $(BUILD_DIR)
	$(ARM_CC) $(CFLAGS) $< -o $@

$(CLIENT_BIN): $(CLIENT_SRC)
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all server client clean