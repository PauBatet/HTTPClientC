# Project structure
SRC_DIR = ./
CACHE_DIR = $(SRC_DIR)/.cache
ENGINE_DIR = $(SRC_DIR)/.engine
HTTP_SERVER_DIR = $(ENGINE_DIR)/HTTPServer
HTML_TEMPLATING_DIR = $(ENGINE_DIR)/HTMLTemplating
BUILD_DIR = $(CACHE_DIR)/build

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g -Wa,--noexecstack \
					-I$(SRC_DIR) \
					-I$(CACHE_DIR) \
					-I$(ENGINE_DIR) \
					-I$(HTML_TEMPLATING_DIR) \
					-I$(HTTP_SERVER_DIR)

# Source files
SRCS = \
		$(ENGINE_DIR)/main.c \
		$(SRC_DIR)/config.c \
		$(HTML_TEMPLATING_DIR)/HTMLTemplating.c \
		$(HTTP_SERVER_DIR)/HTTPServer.c \
		$(SRC_DIR)/routes.c

# Object files
OBJS = \
		$(BUILD_DIR)/main.o \
		$(BUILD_DIR)/config.o \
		$(BUILD_DIR)/HTMLTemplating.o \
		$(BUILD_DIR)/HTTPServer.o \
		$(BUILD_DIR)/routes.o

# Output binary
TARGET = $(BUILD_DIR)/server

# Default target
all: $(TARGET)

# Link final binary
$(TARGET): $(OBJS)
		$(CC) $(CFLAGS) -o $@ $^

$(BUILD_DIR)/main.o: $(ENGINE_DIR)/main.c | $(BUILD_DIR)
		$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/config.o: $(SRC_DIR)/config.c | $(BUILD_DIR)
		$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/HTMLTemplating.o: $(HTML_TEMPLATING_DIR)/HTMLTemplating.c | $(BUILD_DIR)
		$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/HTTPServer.o: $(HTTP_SERVER_DIR)/HTTPServer.c | $(BUILD_DIR)
		$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/routes.o: $(SRC_DIR)/routes.c | $(BUILD_DIR)
		$(CC) $(CFLAGS) -c $< -o $@

# Create build directory
$(BUILD_DIR):
		mkdir -p $(BUILD_DIR)

clean:
		rm -rf $(CACHE_DIR)

run: $(TARGET)
		mkdir -p $(BUILD_DIR)
		./$(TARGET)

.PHONY: all clean run
