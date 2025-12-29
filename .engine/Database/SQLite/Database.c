#include "Database.h"
#include "sqlite3.h"
#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

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

/* -------------------- Transactions with depth -------------------- */

bool db_begin(Database *db)
{
    if (!db) return false;

    if (db->tx_depth == 0) {
        // Only send BEGIN if we’re not already in a transaction
        if (!db_exec(db, "BEGIN")) return false;
    } else {
        // Optional: implement SAVEPOINTs here for nested transactions
        char sql[64];
        snprintf(sql, sizeof(sql), "SAVEPOINT sp_%d", db->tx_depth);
        if (!db_exec(db, sql)) return false;
    }

    db->tx_depth++;
    return true;
}

bool db_commit(Database *db)
{
    if (!db) return false;

    if (db->tx_depth <= 0) {
        fprintf(stderr, "db_commit called without a matching db_begin\n");
        return false;
    }

    db->tx_depth--;

    if (db->tx_depth == 0) {
        return db_exec(db, "COMMIT");
    } else {
        // Optional: nothing, or release savepoint
        char sql[64];
        snprintf(sql, sizeof(sql), "RELEASE SAVEPOINT sp_%d", db->tx_depth);
        return db_exec(db, sql);
    }
}

bool db_rollback(Database *db)
{
    if (!db) return false;

    if (db->tx_depth <= 0) {
        fprintf(stderr, "db_rollback called without a matching db_begin\n");
        return false;
    }

    db->tx_depth--;

    if (db->tx_depth == 0) {
        return db_exec(db, "ROLLBACK");
    } else {
        // Roll back to last savepoint
        char sql[64];
        snprintf(sql, sizeof(sql), "ROLLBACK TO SAVEPOINT sp_%d", db->tx_depth);
        return db_exec(db, sql);
    }
}
