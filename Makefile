# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g -I$(SRC_DIR)

# Project structure
SRC_DIR = ./
HTTP_SERVER_DIR = $(SRC_DIR)/HTTPServer
BUILD_DIR = build

# Source files
SRCS = $(SRC_DIR)/main.c $(SRC_DIR)/routes.c $(HTTP_SERVER_DIR)/HTTPServer.c
OBJS = $(BUILD_DIR)/main.o $(BUILD_DIR)/routes.o $(BUILD_DIR)/HTTPServer.o

# Output binary
TARGET = $(BUILD_DIR)/server

# Default target
all: $(TARGET)

# Create output binary
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Compile main.o
$(BUILD_DIR)/main.o: $(SRC_DIR)/main.c $(SRC_DIR)/routes.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile routes.o (includes views.c)
$(BUILD_DIR)/routes.o: $(SRC_DIR)/routes.c $(SRC_DIR)/views.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile HTTPServer.o
$(BUILD_DIR)/HTTPServer.o: $(HTTP_SERVER_DIR)/HTTPServer.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Create build directory if it doesn't exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Clean build directory
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

# Run the server
run: $(TARGET)
	./$(TARGET)

# Phony targets
.PHONY: all clean

