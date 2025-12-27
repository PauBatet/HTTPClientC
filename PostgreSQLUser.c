#include ".cache/models/User.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../.engine/Database/Database.h"

bool User_create(Database *db, User *obj) {
    char sql[1024];
    snprintf(sql, sizeof(sql), "INSERT INTO \"User\" (\"DNI\", \"name\", \"age\", \"email\", \"group_id\") VALUES ('%s', '%s', %d, '%s', %d)", obj->DNI, obj->name, obj->age, obj->email, obj->group_id);
    return db_exec_safe(db, sql);
}

bool User_create_many(Database *db, UserList *list) {
    if (!db_exec_safe(db, "BEGIN")) return false;
    for (size_t i = 0; i < list->count; i++) {
        if (!User_create(db, &list->items[i])) {
            db_exec_safe(db, "ROLLBACK");
            return false;
        }
    }
    return db_exec_safe(db, "COMMIT");
}

bool User_read(Database *db, char* DNI, User *obj) {
    User_free(obj);
    memset(obj, 0, sizeof(*obj));
    DBResult *r;
    char sql[512];
    snprintf(sql, sizeof(sql), "SELECT \"DNI\", \"name\", \"age\", \"email\", \"group_id\" FROM \"User\" WHERE \"DNI\" = '%s'", DNI);
    if (!db_query_safe(db, sql, &r)) return false;
    bool ok = false;
    if (db_result_next(r)) {
        obj->DNI = db_result_string(r, 0);
        obj->name = db_result_string(r, 1);
        obj->age = db_result_int(r, 2);
        obj->email = db_result_string(r, 3);
        obj->group_id = db_result_int(r, 4);
        ok = true;
    }
    db_result_free(r);
    return ok;
}

bool User_read_all(Database *db, UserList *out) {
    DBResult *r;
    const char *sql = "SELECT \"DNI\", \"name\", \"age\", \"email\", \"group_id\" FROM \"User\"";
    if (!db_query_safe(db, sql, &r)) return false;
    size_t cap = 8;
    out->count = 0;
    out->items = calloc(cap, sizeof(User));
    while (db_result_next(r)) {
        if (out->count == cap) {
            cap *= 2;
            void *tmp = realloc(out->items, sizeof(User) * cap);
            if (!tmp) {
                for (size_t j = 0; j < out->count; j++) User_free(&out->items[j]);
                free(out->items);
                out->items = NULL;
                out->count = 0;
                db_result_free(r);
                return false;
            }
            out->items = tmp;
        }
        User *obj = &out->items[out->count++];
        obj->DNI = db_result_string(r, 0);
        obj->name = db_result_string(r, 1);
        obj->age = db_result_int(r, 2);
        obj->email = db_result_string(r, 3);
        obj->group_id = db_result_int(r, 4);
    }
    db_result_free(r);
    return true;
}

bool User_query(Database *db, const char *where, UserList *out) {
    char sql[1024];
    DBResult *r;
    snprintf(sql, sizeof(sql), "SELECT \"DNI\", \"name\", \"age\", \"email\", \"group_id\" FROM \"User\" WHERE %s", where);
    if (!db_query_safe(db, sql, &r)) return false;
    size_t cap = 8;
    out->count = 0;
    out->items = calloc(cap, sizeof(User));
    while (db_result_next(r)) {
        if (out->count == cap) {
            cap *= 2;
            void *tmp = realloc(out->items, sizeof(User) * cap);
            if (!tmp) {
                for (size_t j = 0; j < out->count; j++) User_free(&out->items[j]);
                free(out->items);
                out->items = NULL;
                out->count = 0;
                db_result_free(r);
                return false;
            }
            out->items = tmp;
        }
        User *obj = &out->items[out->count++];
        obj->DNI = db_result_string(r, 0);
        obj->name = db_result_string(r, 1);
        obj->age = db_result_int(r, 2);
        obj->email = db_result_string(r, 3);
        obj->group_id = db_result_int(r, 4);
    }
    db_result_free(r);
    return true;
}

void User_free(User *obj) {
    if (!obj) return;
    free(obj->DNI);
    free(obj->name);
    free(obj->email);
}

void UserList_free(UserList *list) {
    for (size_t i = 0; i < list->count; i++) {
        User_free(&list->items[i]);
    }
    free(list->items);
    list->items = NULL;
    list->count = 0;
}
bool User_update(Database *db, User *obj) {
    char sql[1024];
    snprintf(sql, sizeof(sql), "UPDATE \"User\" SET \"name\"='%s', \"age\"=%d, \"email\"='%s', \"group_id\"=%d WHERE \"DNI\" = '%s'", obj->name, obj->age, obj->email, obj->group_id, obj->DNI);
    return db_exec_safe(db, sql);
}

bool User_update_many(Database *db, UserList *list) {
    if (!db_exec_safe(db, "BEGIN")) return false;
    for (size_t i = 0; i < list->count; i++) {
        if (!User_update(db, &list->items[i])) {
            db_exec_safe(db, "ROLLBACK");
            return false;
        }
    }
    return db_exec_safe(db, "COMMIT");
}

bool User_delete(Database *db, char* DNI) {
    char sql[256];
    snprintf(sql, sizeof(sql), "DELETE FROM \"User\" WHERE \"DNI\" = '%s'", DNI);
    return db_exec_safe(db, sql);
}
bool User_delete_many(Database *db, UserList *list) {
    if (!db_exec_safe(db, "BEGIN")) return false;
    for (size_t i = 0; i < list->count; i++) {
        if (!User_delete(db, list->items[i].DNI)) {
            db_exec_safe(db, "ROLLBACK");
            return false;
        }
    }
    return db_exec_safe(db, "COMMIT");
}

