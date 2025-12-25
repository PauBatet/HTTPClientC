#include "Database.h"
#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <libpq-fe.h>

static pthread_mutex_t db_mutex = PTHREAD_MUTEX_INITIALIZER;

struct Database {
    PGconn *conn;
};

struct DBResult {
    PGresult *res;
    int current_row;
};

/* -------------------- Lifecycle -------------------- */

bool db_open(Database **db) {
    *db = malloc(sizeof(Database));
    if (!*db) return false;

    char conninfo[512];
    snprintf(conninfo, sizeof(conninfo),
             "host=%s port=%d dbname=%s user=%s password=%s",
             PG_HOST, PG_PORT, PG_DBNAME, PG_USER, PG_PASSWORD);

    (*db)->conn = PQconnectdb(conninfo);
    if (PQstatus((*db)->conn) != CONNECTION_OK) {
        fprintf(stderr, "Connection to database failed: %s\n", PQerrorMessage((*db)->conn));
        free(*db);
        return false;
    }

    return true;
}

void db_close(Database *db) {
    if (!db) return;
    PQfinish(db->conn);
    free(db);
}

/* -------------------- Simple exec -------------------- */

bool db_exec(Database *db, const char *sql)
{
    PGresult *res = PQexec(db->conn, sql);
    if (PQresultStatus(res) != PGRES_COMMAND_OK && PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "SQL error: %s\nSQL: %s\n", PQerrorMessage(db->conn), sql);
        PQclear(res);
        return false;
    }
    PQclear(res);
    return true;
}

/* -------------------- Query helpers -------------------- */

bool db_query(Database *db, const char *sql, DBResult **out)
{
    PGresult *res = PQexec(db->conn, sql);
    if (!res || (PQresultStatus(res) != PGRES_TUPLES_OK && PQresultStatus(res) != PGRES_COMMAND_OK)) {
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

bool db_result_next(DBResult *r)
{
    if (!r) return false;
    r->current_row++;
    return r->current_row < PQntuples(r->res);
}

int db_result_int(DBResult *r, int col)
{
    if (!r) return 0;
    char *val = PQgetvalue(r->res, r->current_row, col);
    return val ? atoi(val) : 0;
}

double db_result_double(DBResult *r, int col)
{
    if (!r) return 0.0;
    char *val = PQgetvalue(r->res, r->current_row, col);
    return val ? atof(val) : 0.0;
}

char *db_result_string(DBResult *r, int col)
{
    if (!r) return NULL;
    char *val = PQgetvalue(r->res, r->current_row, col);
    return val ? strdup(val) : NULL;
}

void db_result_free(DBResult *r)
{
    if (!r) return;
    PQclear(r->res);
    free(r);
}

/* -------------------- Thread-safe helpers -------------------- */

bool db_exec_safe(Database *db, const char *sql)
{
    pthread_mutex_lock(&db_mutex);
    bool ok = db_exec(db, sql);
    pthread_mutex_unlock(&db_mutex);
    return ok;
}

bool db_query_safe(Database *db, const char *sql, DBResult **r)
{
    pthread_mutex_lock(&db_mutex);
    bool ok = db_query(db, sql, r);
    pthread_mutex_unlock(&db_mutex);
    return ok;
}

