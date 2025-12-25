#include "config.h"

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
const char *DB_BACKEND = DB_POSTGRES;

// PostgreSQL connection info
const char *PG_HOST     = "127.0.0.1";
const int   PG_PORT     = 5432;
const char *PG_DBNAME   = "appdb";
const char *PG_USER     = "appuser";
const char *PG_PASSWORD = "apppassword";

// SQLite file path
const char *SQLITE_PATH = "app.db";
