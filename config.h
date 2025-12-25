#ifndef CONFIG_H
#define CONFIG_H

// Server settings
extern const int SERVER_PORT;
extern const char *TEMPLATE_DIR;
extern const int NUM_WORKERS;

// Models
extern const char *MODEL_PATHS[];
extern const int NUM_MODEL_DIRS;

// DB backend
extern const char *DB_BACKEND;

// DB backend "enum" via macros
#define DB_SQLITE "sqlite"
#define DB_POSTGRES "postgres"

// PostgreSQL connection params
extern const char *PG_HOST;
extern const int   PG_PORT;
extern const char *PG_DBNAME;
extern const char *PG_USER;
extern const char *PG_PASSWORD;

// SQLite path
extern const char *SQLITE_PATH;

#endif
