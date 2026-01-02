## Paths
SHELL                := /bin/bash
SRC_DIR              := ./
CACHE_DIR            := $(SRC_DIR).cache
ENGINE_DIR           := $(SRC_DIR).engine
HTTP_SERVER_DIR      := $(ENGINE_DIR)/HTTPServer
HTML_TEMPLATING_DIR  := $(ENGINE_DIR)/HTMLTemplating
DATABASE_DIR         := $(ENGINE_DIR)/Database
ROUTING_DIR          := $(ENGINE_DIR)/Routing
MODEL_DIR            := $(ENGINE_DIR)/Models
BUILD_DIR            := $(CACHE_DIR)/build

# Ensure dirs exist (best-effort at parse-time)
$(shell mkdir -p $(CACHE_DIR) $(BUILD_DIR))

# Compiler
CC     := gcc

LDFLAGS += -Wl,-z,noexecstack

CFLAGS := -Wall -Wextra -g -Wa,--noexecstack \
          -I$(SRC_DIR) -I$(CACHE_DIR) -I$(ENGINE_DIR) \
          -I$(HTML_TEMPLATING_DIR) -I$(HTTP_SERVER_DIR) -I$(DATABASE_DIR) -I$(ROUTING_DIR)

CFLAGS += -I/usr/include/postgresql

# Server sources (core sources compiled at link-time)
SRCS := $(ENGINE_DIR)/main.c \
        $(SRC_DIR)/config.c \
        $(HTML_TEMPLATING_DIR)/HTMLTemplating.c \
        $(HTTP_SERVER_DIR)/HTTPServer.c \
        $(ROUTING_DIR)/Routing.c \
        $(SRC_DIR)/routes.c

TARGET := $(BUILD_DIR)/server


# ------------------------------------------------------------
# Utility targets: create cache files from config.c
# - .cache/model_paths => lines with directories (from MODEL_PATHS[] in config.c)
# - .cache/db_backend => single word: sqlite or postgres (from DB_BACKEND in config.c)
# ------------------------------------------------------------

$(CACHE_DIR)/model_paths: $(SRC_DIR)/config.c | $(CACHE_DIR)
	@echo "Generating model paths from config.c..."
	@TMP=$$(mktemp -t gen_model_paths.XXXX.c); \
	printf '%s\n' '#include "config.c"' > $$TMP; \
	printf '%s\n' '#include <stdio.h>' >> $$TMP; \
	printf '%s\n' 'int main() { int i=0; while (MODEL_PATHS[i]) { printf("%s\n", MODEL_PATHS[i++]); } return 0; }' >> $$TMP; \
	$(CC) $(CFLAGS) $$TMP -o $(CACHE_DIR)/gen_model_paths; \
	./$(CACHE_DIR)/gen_model_paths > $(CACHE_DIR)/model_paths || true; \
	rm -f $$TMP $(CACHE_DIR)/gen_model_paths; \
	echo "-> $(CACHE_DIR)/model_paths generated."

$(CACHE_DIR)/db_backend: $(SRC_DIR)/config.c | $(CACHE_DIR)
	@echo "Reading DB backend from config.c..."
	@TMP=$$(mktemp -t gen_db_backend.XXXX.c); \
	printf '%s\n' '#include "config.c"' > $$TMP; \
	printf '%s\n' '#include <stdio.h>' >> $$TMP; \
	printf '%s\n' 'int main() { printf("%s", DB_BACKEND); return 0; }' >> $$TMP; \
	$(CC) $(CFLAGS) $$TMP -o $(CACHE_DIR)/gen_db_backend; \
	./$(CACHE_DIR)/gen_db_backend > $(CACHE_DIR)/db_backend || true; \
	rm -f $$TMP $(CACHE_DIR)/gen_db_backend; \
	echo "-> $(CACHE_DIR)/db_backend contains: $$(cat $(CACHE_DIR)/db_backend 2>/dev/null)"

# Ensure cache dir target
$(CACHE_DIR):
	mkdir -p $(CACHE_DIR)

# ------------------------------------------------------------
# Migrate: compile engine Models.c + user model .c files
# ------------------------------------------------------------

migrate: $(CACHE_DIR)/model_paths $(CACHE_DIR)/db_backend
	@echo "Running migrate: building migration binary and executing it..."
	@mkdir -p $(CACHE_DIR)/models
	@DB_BACKEND=$$(cat $(CACHE_DIR)/db_backend 2>/dev/null || echo ""); \
	[ -n "$$DB_BACKEND" ] || (echo "ERROR: db backend not set; run 'make $(CACHE_DIR)/db_backend' or check config.c"; exit 1); \
	MODEL_SRCS=$$(xargs -a $(CACHE_DIR)/model_paths -I {} find {} -maxdepth 1 -type f -name '*.c' 2>/dev/null || true); \
	[ -f "$(SRC_DIR)models.c" ] && MODEL_SRCS="$$MODEL_SRCS $(SRC_DIR)models.c"; \
	echo " DB backend: $$DB_BACKEND"; \
	echo " Model sources: $$MODEL_SRCS"; \
	if [ "$$DB_BACKEND" = "sqlite" ]; then \
		DB_SRC="$(DATABASE_DIR)/SQLite/Database.c"; \
		DB_LIBS="-lsqlite3"; \
		MD_SRC="$(MODEL_DIR)/SQLite/Models.c"; \
	elif [ "$$DB_BACKEND" = "postgres" ]; then \
		DB_SRC="$(DATABASE_DIR)/PostgreSQL/Database.c"; \
		DB_LIBS="-lpq"; \
		MD_SRC="$(MODEL_DIR)/PostgreSQL/Models.c"; \
	else \
		echo "Unknown DB_BACKEND: $$DB_BACKEND"; exit 1; \
	fi; \
	$(CC) $(CFLAGS) -o $(CACHE_DIR)/models/migrate \
		$$MD_SRC $(SRC_DIR)/config.c $$DB_SRC $$MODEL_SRCS $$DB_LIBS $(LDFLAGS) || exit 1; \
	./$(CACHE_DIR)/models/migrate || (echo "Migration binary failed"; exit 1); \
	echo "Migration finished."

# ------------------------------------------------------------
# Build/run server
# ------------------------------------------------------------

$(TARGET):
	@echo "Building server (no migrate auto-run)."
	@if [ ! -f GeneratedModels.h ]; then echo "❌ GeneratedModels.h missing — run 'make migrate' first"; exit 1; fi
	@DB_BACKEND=$$(cat $(CACHE_DIR)/db_backend 2>/dev/null || echo ""); \
	[ -n "$$DB_BACKEND" ] || (echo "ERROR: db backend not set; run 'make $(CACHE_DIR)/db_backend' or check config.c"; exit 1); \
	MODEL_C=$$(ls -1 $(CACHE_DIR)/models/*.c 2>/dev/null || true); \
	OBJS=""; \
	for f in $$MODEL_C; do \
		base=$$(basename $$f .c); \
		OBJ=$(BUILD_DIR)/models_$$base.o; \
		echo " compiling model $$f -> $$OBJ"; \
		$(CC) $(CFLAGS) -c $$f -o $$OBJ || exit 1; \
		OBJS="$$OBJS $$OBJ"; \
	done; \
	if [ "$$DB_BACKEND" = "sqlite" ]; then \
		RUNTIME_DB_SRC="$(DATABASE_DIR)/SQLite/Database.c"; \
		DB_LIBS="-lsqlite3"; \
	elif [ "$$DB_BACKEND" = "postgres" ]; then \
		RUNTIME_DB_SRC="$(DATABASE_DIR)/PostgreSQL/Database.c"; \
		DB_LIBS="-lpq"; \
	else \
		echo "Unknown DB_BACKEND: $$DB_BACKEND"; exit 1; \
	fi; \
	echo "Linking server with DB backend $$DB_BACKEND ..."; \
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) $$RUNTIME_DB_SRC $$OBJS $$DB_LIBS $(LDFLAGS) || { echo "Link failed"; exit 1; }; \
	echo "Server built: $(TARGET)"

all: $(TARGET)
	@echo "Making server..."
	@mkdir -p $(BUILD_DIR)

run: $(TARGET)
	@echo "Starting server..."
	@mkdir -p $(BUILD_DIR)
	@./$(TARGET)

# ------------------------------------------------------------
# Clean
# ------------------------------------------------------------

clean:
	rm -rf $(BUILD_DIR)

full_clean:
	rm -rf $(CACHE_DIR)
	rm -f GeneratedModels.h

.PHONY: clean_test
clean_test:
	@rm -rf $(TEST_BUILD_DIR)
	@rm -rf $(CACHE_DIR)/tests
	@rm -f test.db 
