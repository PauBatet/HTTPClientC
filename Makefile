## Paths
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
CFLAGS := -Wall -Wextra -g -Wa,--noexecstack \
          -I$(SRC_DIR) -I$(CACHE_DIR) -I$(ENGINE_DIR) \
          -I$(HTML_TEMPLATING_DIR) -I$(HTTP_SERVER_DIR) -I$(DATABASE_DIR) -I$(ROUTING_DIR)

# Server sources (core sources compiled at link-time)
SRCS := $(ENGINE_DIR)/main.c \
        $(SRC_DIR)/config.c \
        $(HTML_TEMPLATING_DIR)/HTMLTemplating.c \
        $(HTTP_SERVER_DIR)/HTTPServer.c \
        $(ROUTING_DIR)/Routing.c \
        $(SRC_DIR)/routes.c

TARGET := $(BUILD_DIR)/server

.PHONY: all clean full_clean migrate run

all: $(TARGET)

# ------------------------------------------------------------
# Utility targets: create cache files from config.c
# - .cache/model_paths => lines with directories (from MODEL_PATHS[] in config.c)
# - .cache/db_backend => single word: sqlite or postgres (from DB_BACKEND in config.c)
# ------------------------------------------------------------

$(CACHE_DIR)/model_paths: $(SRC_DIR)/config.c | $(CACHE_DIR)
	@echo "Generating model paths from config.c..."
	@TMP=$$(mktemp /tmp/gen_model_paths.XXXX.c); \
	printf '%s\n' '#include "config.c"' > $$TMP; \
	printf '%s\n' '#include <stdio.h>' >> $$TMP; \
	printf '%s\n' 'int main() { int i=0; while (MODEL_PATHS[i]) { printf("%s\n", MODEL_PATHS[i++]); } return 0; }' >> $$TMP; \
	$(CC) $(CFLAGS) $$TMP -o $(CACHE_DIR)/gen_model_paths; \
	./$(CACHE_DIR)/gen_model_paths > $(CACHE_DIR)/model_paths || true; \
	rm -f $$TMP $(CACHE_DIR)/gen_model_paths; \
	echo "-> $(CACHE_DIR)/model_paths generated."

$(CACHE_DIR)/db_backend: $(SRC_DIR)/config.c | $(CACHE_DIR)
	@echo "Reading DB backend from config.c..."
	@TMP=$$(mktemp /tmp/gen_db_backend.XXXX.c); \
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
		DB_SRC="$(DATABASE_DIR)/SQLite/Database.c $(DATABASE_DIR)/SQLite/sqlite3.c"; \
		DB_LIBS=""; \
		MD_SRC="$(MODEL_DIR)/SQLite/Models.c"; \
	elif [ "$$DB_BACKEND" = "postgres" ]; then \
		DB_SRC="$(DATABASE_DIR)/PostgreSQL/Database.c"; \
		DB_LIBS="-lpq"; \
		MD_SRC="$(MODEL_DIR)/PostgreSQL/Models.c"; \
	else \
		echo "Unknown DB_BACKEND: $$DB_BACKEND"; exit 1; \
	fi; \
	$(CC) $(CFLAGS) -o $(CACHE_DIR)/models/migrate \
		$$MD_SRC $(SRC_DIR)/config.c $$DB_SRC $$MODEL_SRCS $$DB_LIBS || exit 1; \
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
		RUNTIME_DB_SRC="$(DATABASE_DIR)/SQLite/Database.c $(DATABASE_DIR)/SQLite/sqlite3.c"; \
		DB_LIBS=""; \
	elif [ "$$DB_BACKEND" = "postgres" ]; then \
		RUNTIME_DB_SRC="$(DATABASE_DIR)/PostgreSQL/Database.c"; \
		DB_LIBS="-lpq"; \
	else \
		echo "Unknown DB_BACKEND: $$DB_BACKEND"; exit 1; \
	fi; \
	echo "Linking server with DB backend $$DB_BACKEND ..."; \
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) $$RUNTIME_DB_SRC $$OBJS $$DB_LIBS || { echo "Link failed"; exit 1; }; \
	echo "Server built: $(TARGET)"

run: $(TARGET)
	@echo "Starting server..."
	@mkdir -p $(BUILD_DIR)
	@./$(TARGET)

# ------------------------------------------------------------
# Tests
# ------------------------------------------------------------

TEST_DIR        := $(SRC_DIR)tests
TEST_BUILD_DIR  := $(BUILD_DIR)/tests
UNITY_ROOT      := ./tests/unity
TEST_FILES      := $(wildcard $(TEST_DIR)/test_*.c)

$(TEST_BUILD_DIR):
	mkdir -p $(TEST_BUILD_DIR)

$(CACHE_DIR)/tests:
	mkdir -p $(CACHE_DIR)/tests
$(CACHE_DIR)/tests/mock_db_backend: $(SRC_DIR)/tests/mock_config.c | $(CACHE_DIR)/tests
	@echo "Reading DB backend from config.c..."
	@TMP=$$(mktemp /tmp/gen_db_backend.XXXX.c); \
	printf '%s\n' '#include "tests/mock_config.c"' > $$TMP; \
	printf '%s\n' '#include <stdio.h>' >> $$TMP; \
	printf '%s\n' 'int main() { printf("%s", DB_BACKEND); return 0; }' >> $$TMP; \
	$(CC) $(CFLAGS) $$TMP -o $(CACHE_DIR)/gen_db_backend; \
	./$(CACHE_DIR)/gen_db_backend > $(CACHE_DIR)/tests/mock_db_backend || true; \
	rm -f $$TMP $(CACHE_DIR)/gen_db_backend; \
	echo "-> $(CACHE_DIR)/tests/mock_db_backend contains: $$(cat $(CACHE_DIR)/tests/mock_db_backend 2>/dev/null)"

.PHONY: test_migrate
test_migrate: $(CACHE_DIR)/tests/mock_db_backend
	@echo "🛠️ Running TEST Migration (Mock Models)..."
	@mkdir -p $(CACHE_DIR)/models
	@DB_BACKEND=$$(cat $(CACHE_DIR)/tests/mock_db_backend 2>/dev/null || echo "postgres"); \
	if [ "$$DB_BACKEND" = "sqlite" ]; then \
		DB_SRC="$(DATABASE_DIR)/SQLite/Database.c $(DATABASE_DIR)/SQLite/sqlite3.c"; \
		MD_SRC="$(MODEL_DIR)/SQLite/Models.c"; \
	else \
		DB_SRC="$(DATABASE_DIR)/PostgreSQL/Database.c"; \
		MD_SRC="$(MODEL_DIR)/PostgreSQL/Models.c"; \
	fi; \
	$(CC) $(CFLAGS) -o $(CACHE_DIR)/models/test_migrate \
		$$MD_SRC $(TEST_DIR)/mock_config.c $$DB_SRC $(TEST_DIR)/mock_models.c -lpq || exit 1; \
	./$(CACHE_DIR)/models/test_migrate || exit 1;
	@echo "✅ Test Migration finished. Mock models generated."

.PHONY: test
test: test_migrate $(TEST_BUILD_DIR)
	@echo "🧪 Starting Test Suite..."
	@DB_BACKEND=$$(cat $(CACHE_DIR)/db_backend 2>/dev/null); \
	if [ "$$DB_BACKEND" = "sqlite" ]; then \
		DB_FILES="$(DATABASE_DIR)/SQLite/Database.c $(DATABASE_DIR)/SQLite/sqlite3.c"; \
		DB_LIBS=""; \
	else \
		DB_FILES="$(DATABASE_DIR)/PostgreSQL/Database.c"; \
		DB_LIBS="-lpq"; \
	fi; \
	T_CFLAGS="$(CFLAGS) -I$(UNITY_ROOT) -DUNIT_TEST"; \
	T_LIBS="-lpthread -ldl $$DB_LIBS"; \
	for test_file in $(TEST_FILES); do \
		test_name=$$(basename $$test_file .c); \
		echo "\n--------------------------------------------------"; \
		echo "🛠️  Compiling $$test_name..."; \
		GEN_MODELS=$$(ls $(CACHE_DIR)/models/*.c 2>/dev/null || true); \
		$(CC) $$T_CFLAGS $(UNITY_ROOT)/unity.c $(TEST_DIR)/mock_config.c \
			$$GEN_MODELS $$DB_FILES $$test_file \
			-o $(TEST_BUILD_DIR)/$$test_name $$T_LIBS || exit 1; \
		echo "🚀 Running $$test_name..."; \
		$(TEST_BUILD_DIR)/$$test_name || exit 1; \
	done; \
	echo "\n✅ All tests passed!"

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
	@rm -f test.db 
