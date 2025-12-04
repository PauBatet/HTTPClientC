#include "Database.h"
#include "sqlite3.h"
#include <stdlib.h>
#include <stdio.h>

struct Database {
    sqlite3 *conn;
};

bool db_open(Database **db, const char *path)
{
    *db = malloc(sizeof(Database));
    if (!*db) return false;

    if (sqlite3_open(path, &(*db)->conn) != SQLITE_OK) {
        free(*db);
        return false;
    }
    return true;
}

void db_close(Database *db)
{
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

