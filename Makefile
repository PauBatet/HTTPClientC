# Project structure
SRC_DIR = ./
CACHE_DIR = $(SRC_DIR).cache
ENGINE_DIR = $(SRC_DIR).engine
HTTP_SERVER_DIR = $(ENGINE_DIR)/HTTPServer
HTML_TEMPLATING_DIR = $(ENGINE_DIR)/HTMLTemplating
DATABASE_DIR = $(ENGINE_DIR)/Database
ROUTING_DIR = $(ENGINE_DIR)/Routing
BUILD_DIR = $(CACHE_DIR)/build
MODEL_SRCS := $(wildcard $(CACHE_DIR)/models/*.c)
MODEL_OBJS := $(patsubst $(CACHE_DIR)/models/%.c,$(BUILD_DIR)/models_%.o,$(MODEL_SRCS))

$(shell mkdir -p $(CACHE_DIR) $(BUILD_DIR))

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g -Wa,--noexecstack \
					-I$(SRC_DIR) \
					-I$(CACHE_DIR) \
					-I$(ENGINE_DIR) \
					-I$(HTML_TEMPLATING_DIR) \
					-I$(HTTP_SERVER_DIR) \
					-I$(DATABASE_DIR) \
					-I$(ROUTING_DIR)

# Source files
SRCS = \
		$(ENGINE_DIR)/main.c \
		$(SRC_DIR)/config.c \
		$(HTML_TEMPLATING_DIR)/HTMLTemplating.c \
		$(HTTP_SERVER_DIR)/HTTPServer.c \
		$(DATABASE_DIR)/Database.c \
		$(DATABASE_DIR)/sqlite3.c \
		$(ROUTING_DIR)/Routing.c \
		$(SRC_DIR)/routes.c

# Object files
OBJS = \
		$(BUILD_DIR)/main.o \
		$(BUILD_DIR)/config.o \
		$(BUILD_DIR)/HTMLTemplating.o \
		$(BUILD_DIR)/HTTPServer.o \
		$(BUILD_DIR)/Database.o \
		$(BUILD_DIR)/sqlite3.o \
		$(BUILD_DIR)/Routing.o \
		$(BUILD_DIR)/routes.o

OBJS += $(MODEL_OBJS)

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

$(BUILD_DIR)/models.o: $(SRC_DIR)/models.c | $(BUILD_DIR)
		$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/HTMLTemplating.o: $(HTML_TEMPLATING_DIR)/HTMLTemplating.c | $(BUILD_DIR)
		$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/HTTPServer.o: $(HTTP_SERVER_DIR)/HTTPServer.c | $(BUILD_DIR)
		$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/Database.o: $(DATABASE_DIR)/Database.c | $(BUILD_DIR)
		$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/sqlite3.o: $(DATABASE_DIR)/sqlite3.c | $(BUILD_DIR)
		$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/Routing.o: $(ROUTING_DIR)/Routing.c | $(BUILD_DIR)
		$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/routes.o: $(SRC_DIR)/routes.c | $(BUILD_DIR)
		$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/models_%.o: $(CACHE_DIR)/models/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Create build directory
$(BUILD_DIR):
		mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

full_clean:
		rm -rf $(CACHE_DIR)
		rm -f GeneratedModels.h

run: $(TARGET)
		@test -f GeneratedModels.h || (echo "❌ Run 'make migrate' first"; exit 1)
		mkdir -p $(BUILD_DIR)
		./$(TARGET)

# Generate model paths from config.c
$(CACHE_DIR)/model_paths: $(SRC_DIR)/config.c
	@TMP_MAIN=$(SRC_DIR)/main_tmp.c; \
	echo '#include "config.c"' > $$TMP_MAIN; \
	echo '#include <stdio.h>' >> $$TMP_MAIN; \
	echo 'int main() { int i=0; while(MODEL_PATHS[i]) printf("%s\n", MODEL_PATHS[i++]); return 0; }' >> $$TMP_MAIN; \
	$(CC) $(CFLAGS) $$TMP_MAIN -o $(CACHE_DIR)/gen_paths; \
	./$(CACHE_DIR)/gen_paths > $(CACHE_DIR)/model_paths; \
	rm -f $$TMP_MAIN $(CACHE_DIR)/gen_paths

# Deferred expansion: read model paths at execution time
MODEL_DECL_SRCS = $(shell [ -f $(CACHE_DIR)/model_paths ] && xargs -a $(CACHE_DIR)/model_paths -I {} find {} -name '*.c' || echo "")

# Model object files
MODEL_OBJS = $(patsubst $(CACHE_DIR)/models/%.c,$(BUILD_DIR)/models_%.o,$(wildcard $(CACHE_DIR)/models/*.c))

# Rule to compile generated model objects
$(BUILD_DIR)/models_%.o: $(CACHE_DIR)/models/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Migrate target: generates CRUD model code
migrate: $(CACHE_DIR)/model_paths
	@mkdir -p $(CACHE_DIR)/models
	@MODEL_SRCS=$$(xargs -a $(CACHE_DIR)/model_paths -I {} find {} -name '*.c'); \
	$(CC) $(CFLAGS) -o $(CACHE_DIR)/models/migrate \
		$(ENGINE_DIR)/Models/Models.c \
		$(ENGINE_DIR)/Database/Database.c \
		$(ENGINE_DIR)/Database/sqlite3.c \
		$$MODEL_SRCS; \
	./$(CACHE_DIR)/models/migrate

.PHONY: all clean run
