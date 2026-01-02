#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Server settings
const char *TEMPLATE_DIR = "templates";
const int SERVER_PORT = 8080;
const int NUM_WORKERS = 4;

// Model directories
const char *MODEL_PATHS[] = {
    "models",
};
const int NUM_MODEL_DIRS = 1;

// DB backend
const char *DB_BACKEND = DB_SQLITE;

// PostgreSQL connection info
char *PG_HOST     = "127.0.0.1"; 
int   PG_PORT     = 5432;
char *PG_DBNAME   = "appdb";
char *PG_USER     = "appuser";
char *PG_PASSWORD = "apppassword";
char *PG_SSLMODE  = "disable";

// SQLite file path
char *SQLITE_PATH = "app.db";

void load_config_from_env() {
    char *env_val;

    // Load PostgreSQL Env
    env_val = getenv("PG_HOST");
    if (env_val && strlen(env_val) > 0) PG_HOST = env_val;

    env_val = getenv("PG_DBNAME");
    if (env_val && strlen(env_val) > 0) PG_DBNAME = env_val;

    env_val = getenv("PG_USER");
    if (env_val && strlen(env_val) > 0) PG_USER = env_val;

    env_val = getenv("PG_PASSWORD");
    if (env_val && strlen(env_val) > 0) PG_PASSWORD = env_val;

    env_val = getenv("PG_SSLMODE");
    if (env_val && strlen(env_val) > 0) PG_SSLMODE = env_val;

    env_val = getenv("PG_PORT");
    if (env_val && strlen(env_val) > 0) PG_PORT = atoi(env_val);  

    // Load SQLite Env
    env_val = getenv("SQLITE_PATH");
    if (env_val && strlen(env_val) > 0) SQLITE_PATH = env_val;

    // Smart Logging based on active backend
    printf("--- Configuration Loaded ---\n");
    printf("Backend: %s\n", DB_BACKEND);

    if (strcmp(DB_BACKEND, "postgres") == 0) {
        printf("Postgres: Host=%s, DB=%s, Port=%d, User=%s\n", 
                PG_HOST, PG_DBNAME, PG_PORT, PG_USER);
    } 
    else if (strcmp(DB_BACKEND, "sqlite") == 0) {
        printf("SQLite: Path=%s\n", SQLITE_PATH);
    }
    printf("---------------------------\n");
}
