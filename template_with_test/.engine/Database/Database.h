#pragma once
#include <stdbool.h>
#include "config.h"

typedef struct Database Database;
typedef struct DBResult DBResult;

typedef enum {
    DB_STATUS_OK,
    DB_STATUS_ERROR
} DbStatus;

DbStatus db_get_status(Database *db);

/* Lifecycle */
bool db_open(Database **db);
void db_close(Database *db);

/* Simple exec helper */
//bool db_exec(Database *db, const char *sql);

/* Prepared statement helpers */
//bool db_prepare(Database *db, const char *sql, void **stmt);
//bool db_step(void *stmt);
//void db_finalize(void *stmt);

bool db_exec(Database *db, const char *sql);
bool db_query(Database *db, const char *sql, DBResult **out);

bool db_result_next(DBResult *r);

int    db_result_int(DBResult *r, int col);
double db_result_double(DBResult *r, int col);
char  *db_result_string(DBResult *r, int col);

void db_result_free(DBResult *r);

bool db_exec_safe(Database *db, const char *sql); 

bool db_query_safe(Database *db, const char *sql, DBResult **r);

bool db_exec_params(Database *db, const char *sql, int nparams, const char *params[]);

bool db_query_params(Database *db, const char *sql, int nparams, const char *params[], DBResult **out);
