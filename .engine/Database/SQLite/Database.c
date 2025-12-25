#include "Database.h"
#include "sqlite3.h"
#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

static pthread_mutex_t db_mutex = PTHREAD_MUTEX_INITIALIZER;

struct Database {
    sqlite3 *conn;
};

struct DBResult {
    sqlite3_stmt *stmt;
};

bool db_open(Database **db) {
    *db = malloc(sizeof(Database));
    if (!*db) return false;
    if (sqlite3_open(SQLITE_PATH, &(*db)->conn) != SQLITE_OK) {
        fprintf(stderr, "SQLite open failed: %s\n", sqlite3_errmsg((*db)->conn));
        free(*db);
        return false;
    }

    return true;
}

void db_close(Database *db) {
    if (!db) return;
    sqlite3_close(db->conn);
    free(db);
}

bool db_exec(Database *db, const char *sql) {
    char *err = NULL;
    int rc = sqlite3_exec(db->conn, sql, NULL, NULL, &err);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\nSQL: %s\n", err, sql);
        sqlite3_free(err);
        return false;
    }
    return true;
}

/*
bool db_prepare(Database *db, const char *sql, void **stmt)
{
    return sqlite3_prepare_v2(
        db->conn, sql, -1,
        (sqlite3_stmt **)stmt,
        NULL
    ) == SQLITE_OK;
}

bool db_step(void *stmt)
{
    return sqlite3_step((sqlite3_stmt *)stmt) == SQLITE_ROW;
}

void db_finalize(void *stmt)
{
    sqlite3_finalize((sqlite3_stmt *)stmt);
}
*/

bool db_query(Database *db, const char *sql, DBResult **out) {
    DBResult *r = malloc(sizeof(DBResult));
    if (!r) return false;

    if (sqlite3_prepare_v2(db->conn, sql, -1, &r->stmt, NULL) != SQLITE_OK) {
        free(r);
        return false;
    }

    *out = r;
    return true;
}

bool db_result_next(DBResult *r) {
    return sqlite3_step(r->stmt) == SQLITE_ROW;
}

int db_result_int(DBResult *r, int col) {
    return sqlite3_column_int(r->stmt, col);
}

double db_result_double(DBResult *r, int col) {
    return sqlite3_column_double(r->stmt, col);
}

char *db_result_string(DBResult *r, int col) {
    const unsigned char *txt = sqlite3_column_text(r->stmt, col);
    return txt ? strdup((const char *)txt) : NULL;
}

void db_result_free(DBResult *r) {
    if (!r) return;
    sqlite3_finalize(r->stmt);
    free(r);
}

bool db_exec_safe(Database *db, const char *sql) {
    pthread_mutex_lock(&db_mutex);
    bool ok = db_exec(db, sql);
    pthread_mutex_unlock(&db_mutex);
    return ok;
}

bool db_query_safe(Database *db, const char *sql, DBResult **r) {
    pthread_mutex_lock(&db_mutex);
    bool ok = db_query(db, sql, r);
    pthread_mutex_unlock(&db_mutex);
    return ok;
}
