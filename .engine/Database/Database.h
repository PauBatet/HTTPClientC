#pragma once
#include <stdbool.h>

typedef struct Database Database;

/* Lifecycle */
bool db_open(Database **db, const char *path);
void db_close(Database *db);

/* Simple exec helper */
bool db_exec(Database *db, const char *sql);

/* Prepared statement helpers */
bool db_prepare(Database *db, const char *sql, void **stmt);
bool db_step(void *stmt);
void db_finalize(void *stmt);

