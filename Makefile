# Project structure
SRC_DIR = ./
HTTP_SERVER_DIR = $(SRC_DIR)/HTTPServer
HTML_TEMPLATING_DIR = $(SRC_DIR)/HTMLTemplating
BUILD_DIR = build

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g -Wa,--noexecstack \
         -I$(SRC_DIR) \
         -I$(HTML_TEMPLATING_DIR) \
         -I$(HTTP_SERVER_DIR)

# Source files
SRCS = \
	$(SRC_DIR)/main.c \
	$(SRC_DIR)/routes.c \
	$(SRC_DIR)/views.c \
	$(HTML_TEMPLATING_DIR)/HTMLTemplating.c \
	$(HTTP_SERVER_DIR)/HTTPServer.c

# Object files
OBJS = \
	$(BUILD_DIR)/main.o \
	$(BUILD_DIR)/routes.o \
	$(BUILD_DIR)/views.o \
	$(BUILD_DIR)/HTMLTemplating.o \
	$(BUILD_DIR)/HTTPServer.o

# Output binary
TARGET = $(BUILD_DIR)/server

# Default target
all: $(TARGET)

# Link final binary
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Object rules
$(BUILD_DIR)/main.o: $(SRC_DIR)/main.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/routes.o: $(SRC_DIR)/routes.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/views.o: $(SRC_DIR)/views.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/HTMLTemplating.o: $(HTML_TEMPLATING_DIR)/HTMLTemplating.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/HTTPServer.o: $(HTTP_SERVER_DIR)/HTTPServer.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Clean build directory
clean:
	rm -rf $(BUILD_DIR)

# Run the server
run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run

