#include "Database.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <libpq-fe.h>

struct Database {
    PGconn *conn;
    int tx_depth;
};

struct DBResult {
    PGresult *res;
    int current_row;
};

/* -------------------- Lifecycle -------------------- */

bool db_open(Database **db) {
    *db = malloc(sizeof(Database));
    if (!*db) return false;

    // Safety check: Don't try to connect if HOST is empty
    if (!PG_HOST || strlen(PG_HOST) == 0) {
        fprintf(stderr, "Error: PG_HOST is not set!\n");
        free(*db);
        return false;
    }

    char conninfo[1024]; // Increased size to be safe
    snprintf(conninfo, sizeof(conninfo),
             "host=%s port=%d dbname=%s user=%s password=%s sslmode=%s",
             PG_HOST, PG_PORT, PG_DBNAME, PG_USER, PG_PASSWORD, PG_SSLMODE);

    (*db)->tx_depth = 0;
    (*db)->conn = PQconnectdb(conninfo);
    
    if (PQstatus((*db)->conn) != CONNECTION_OK) {
        fprintf(stderr, "Connection to database failed: %s\n", PQerrorMessage((*db)->conn));
        PQfinish((*db)->conn); // Clean up the failed connection object
        free(*db);
        *db = NULL;
        return false;
    }
    return true;
}

void db_close(Database *db) {
    if (!db) return;
    PQfinish(db->conn);
    free(db);
}

DbStatus db_get_status(Database *db) {
    if (!db || !db->conn) return DB_STATUS_ERROR;
    
    // Check if the connection handle thinks it is okay
    if (PQstatus(db->conn) != CONNECTION_OK) {
        return DB_STATUS_ERROR;
    }

    // Optional: Real-world "ping" to ensure the socket isn't ghosting us
    PGresult *res = PQexec(db->conn, "SELECT 1");
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        if (res) PQclear(res);
        return DB_STATUS_ERROR;
    }
    PQclear(res);
    
    return DB_STATUS_OK;
}

/* -------------------- Safety Wrapper -------------------- */

// Helper to prevent Segfaults when printing errors on dead connections
static void log_db_error(Database *db, const char *context) {
    if (db && db->conn) {
        fprintf(stderr, "DB Error (%s): %s\n", context, PQerrorMessage(db->conn));
    } else {
        fprintf(stderr, "DB Error (%s): Connection is NULL or invalid\n", context);
    }
}

/* -------------------- Simple exec -------------------- */

bool db_exec(Database *db, const char *sql) {
    if (db_get_status(db) != DB_STATUS_OK) return false;

    PGresult *res = PQexec(db->conn, sql);
    ExecStatusType status = PQresultStatus(res);

    if (status != PGRES_COMMAND_OK && status != PGRES_TUPLES_OK) {
        log_db_error(db, sql);
        if (res) PQclear(res);
        return false;
    }
    PQclear(res);
    return true;
}

/* -------------------- Query helpers -------------------- */

bool db_query(Database *db, const char *sql, DBResult **out) {
    if (db_get_status(db) != DB_STATUS_OK || !out) return false;

    PGresult *res = PQexec(db->conn, sql);
    if (!res || (PQresultStatus(res) != PGRES_TUPLES_OK && PQresultStatus(res) != PGRES_COMMAND_OK)) {
        log_db_error(db, sql);
        if (res) PQclear(res);
        return false;
    }

    DBResult *r = malloc(sizeof(DBResult));
    if (!r) {
        PQclear(res);
        return false;
    }

    r->res = res;
    r->current_row = -1;
    *out = r;
    return true;
}

bool db_result_next(DBResult *r) {
    if (!r || !r->res) return false; // Added res check
    r->current_row++;
    return r->current_row < PQntuples(r->res);
}

int db_result_int(DBResult *r, int col) {
    if (!r || !r->res || r->current_row < 0 || r->current_row >= PQntuples(r->res)) return 0;
    char *val = PQgetvalue(r->res, r->current_row, col);
    return val ? atoi(val) : 0;
}

double db_result_double(DBResult *r, int col) {
    if (!r || !r->res || r->current_row < 0 || r->current_row >= PQntuples(r->res)) return 0.0;
    char *val = PQgetvalue(r->res, r->current_row, col);
    return val ? atof(val) : 0.0;
}

char *db_result_string(DBResult *r, int col) {
    if (!r || !r->res || r->current_row < 0 || r->current_row >= PQntuples(r->res)) return NULL;
    char *val = PQgetvalue(r->res, r->current_row, col);
    return val ? strdup(val) : NULL;
}

void db_result_free(DBResult *r)
{
    if (!r) return;
    PQclear(r->res);
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

bool db_exec_params(Database *db, const char *sql, int nparams, const char *params[]) {
    if (db_get_status(db) != DB_STATUS_OK) return false;

    PGresult *res = PQexecParams(db->conn, sql, nparams, NULL, params, NULL, NULL, 0);

    if (!res || PQresultStatus(res) != PGRES_COMMAND_OK) {
        log_db_error(db, sql);
        if (res) PQclear(res);
        return false;
    }

    PQclear(res);
    return true;
}

bool db_query_params(Database *db, const char *sql, int nparams, const char *params[], DBResult **out) {
    if (db_get_status(db) != DB_STATUS_OK || !out) return false;

    PGresult *res = PQexecParams(db->conn, sql, nparams, NULL, params, NULL, NULL, 0);

    if (!res || PQresultStatus(res) != PGRES_TUPLES_OK) {
        log_db_error(db, sql);
        if (res) PQclear(res);
        return false;
    }

    DBResult *r = malloc(sizeof(DBResult));
    if (!r) {
        PQclear(res);
        return false;
    }

    r->res = res;
    r->current_row = -1;
    *out = r;
    return true;
}
