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
extern char *PG_HOST;
extern int   PG_PORT;
extern char *PG_DBNAME;
extern char *PG_USER;
extern char *PG_PASSWORD;
extern char *PG_SSLMODE;

// SQLite path
extern char *SQLITE_PATH;

void load_config_from_env();

#endif
