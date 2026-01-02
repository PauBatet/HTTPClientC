#include ".cache/models/Group.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../.engine/Database/Database.h"

bool Group_create(Database *db, Group *obj) {
    const char *params[2];
    params[0] = obj->name;
    char param_num_members[32]; snprintf(param_num_members, sizeof(param_num_members), "%d", obj->num_members);
    params[1] = param_num_members;
    const char *sql = "INSERT INTO \"Group\" (\"name\", \"num_members\") VALUES (?1, ?2)";
    return db_exec_params(db, sql, 2, params);
}

bool Group_create_many(Database *db, GroupList *list) {
    if (!db_exec(db, "BEGIN")) return false;
    for (size_t i = 0; i < list->count; i++) {
        if (!Group_create(db, &list->items[i])) {
            db_exec(db, "ROLLBACK");
            return false;
        }
    }
    return db_exec(db, "COMMIT");
}

bool Group_read(Database *db, int id, Group *obj) {
    Group_free(obj);
    memset(obj, 0, sizeof(*obj));
    DBResult *r;
    const char *params[1];
    char pk_buf[32]; snprintf(pk_buf, sizeof(pk_buf), "%d", id);
    params[0] = pk_buf;
    const char *sql = "SELECT \"id\", \"name\", \"num_members\" FROM \"Group\" WHERE \"id\" = ?1";
    if (!db_query_params(db, sql, 1, params, &r)) return false;
    bool ok = false;
    if (db_result_next(r)) {
        obj->id = db_result_int(r, 0);
        obj->name = db_result_string(r, 1);
        obj->num_members = db_result_int(r, 2);
        ok = true;
    }
    db_result_free(r);
    return ok;
}

bool Group_read_all(Database *db, GroupList *out) {
    DBResult *r;
    const char *sql = "SELECT \"id\", \"name\", \"num_members\" FROM \"Group\"";
    if (!db_query(db, sql, &r)) return false;
    size_t cap = 8;
    out->count = 0;
    out->items = calloc(cap, sizeof(Group));
    while (db_result_next(r)) {
        if (out->count == cap) {
            cap *= 2;
            void *tmp = realloc(out->items, sizeof(Group) * cap);
            if (!tmp) {
                for (size_t j = 0; j < out->count; j++) Group_free(&out->items[j]);
                free(out->items);
                out->items = NULL;
                out->count = 0;
                db_result_free(r);
                return false;
            }
            out->items = tmp;
        }
        Group *obj = &out->items[out->count++];
        obj->id = db_result_int(r, 0);
        obj->name = db_result_string(r, 1);
        obj->num_members = db_result_int(r, 2);
    }
    db_result_free(r);
    return true;
}

/* WARNING: this function is SQL-injection prone. */
bool Group_query_unsafe(Database *db, const char *where, GroupList *out) {
    char sql[1024];
    DBResult *r;
    snprintf(sql, sizeof(sql), "SELECT \"id\", \"name\", \"num_members\" FROM \"Group\" WHERE %s", where);
    if (!db_query(db, sql, &r)) return false;
    size_t cap = 8;
    out->count = 0;
    out->items = calloc(cap, sizeof(Group));
    while (db_result_next(r)) {
        if (out->count == cap) {
            cap *= 2;
            void *tmp = realloc(out->items, sizeof(Group) * cap);
            if (!tmp) {
                for (size_t j = 0; j < out->count; j++) Group_free(&out->items[j]);
                free(out->items);
                out->items = NULL;
                out->count = 0;
                db_result_free(r);
                return false;
            }
            out->items = tmp;
        }
        Group *obj = &out->items[out->count++];
        obj->id = db_result_int(r, 0);
        obj->name = db_result_string(r, 1);
        obj->num_members = db_result_int(r, 2);
    }
    db_result_free(r);
    return true;
}

void Group_free(Group *obj) {
    if (!obj) return;
    free(obj->name);
}

void GroupList_free(GroupList *list) {
    for (size_t i = 0; i < list->count; i++) {
        Group_free(&list->items[i]);
    }
    free(list->items);
    list->items = NULL;
    list->count = 0;
}
bool Group_update(Database *db, Group *obj) {
    const char *params[3];
    params[0] = obj->name;
    char param_num_members[32]; snprintf(param_num_members, sizeof(param_num_members), "%d", obj->num_members);
    params[1] = param_num_members;
    char pk_buf[32]; snprintf(pk_buf, sizeof(pk_buf), "%d", obj->id);
    params[2] = pk_buf;
    const char *sql = "UPDATE \"Group\" SET \"name\" = ?1, \"num_members\" = ?2 WHERE \"id\" = ?3";
    return db_exec_params(db, sql, 3, params);
}

bool Group_update_many(Database *db, GroupList *list) {
    if (!db_exec(db, "BEGIN")) return false;
    for (size_t i = 0; i < list->count; i++) {
        if (!Group_update(db, &list->items[i])) {
            db_exec(db, "ROLLBACK");
            return false;
        }
    }
    return db_exec(db, "COMMIT");
}

bool Group_delete(Database *db, int id) {
    const char *params[1];
    char pk_buf[32]; snprintf(pk_buf, sizeof(pk_buf), "%d", id);
    params[0] = pk_buf;
    const char *sql = "DELETE FROM \"Group\" WHERE \"id\" = ?1";
    return db_exec_params(db, sql, 1, params);
}

bool Group_delete_many(Database *db, GroupList *list) {
    if (!db_exec(db, "BEGIN")) return false;
    for (size_t i = 0; i < list->count; i++) {
        if (!Group_delete(db, list->items[i].id)) {
            db_exec(db, "ROLLBACK");
            return false;
        }
    }
    return db_exec(db, "COMMIT");
}

