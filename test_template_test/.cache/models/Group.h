#pragma once
#include "../../.engine/Database/Database.h"
#include <stddef.h>

typedef struct {
    int id;
    char* name;
    int num_members;
} Group;

typedef struct {
    Group *items;
    size_t count;
} GroupList;

bool Group_create(Database *db, Group *obj);
bool Group_create_many(Database *db, GroupList *list);

bool Group_read(Database *db, int id, Group *obj);
bool Group_read_all(Database *db, GroupList *out);
bool Group_update(Database *db, Group *obj);
bool Group_update_many(Database *db, GroupList *list);

bool Group_delete(Database *db, int id);
bool Group_delete_many(Database *db, GroupList *list);

bool Group_query_unsafe(Database *db, const char *where, GroupList *out);

void Group_free(Group *obj);
void GroupList_free(GroupList *list);
