#pragma once
#include "../../.engine/Database/Database.h"
#include <stddef.h>

typedef struct {
    char* DNI;
    char* name;
    int age;
    char* email;
    int group_id;
} User;

typedef struct {
    User *items;
    size_t count;
} UserList;

bool User_create(Database *db, User *obj);
bool User_create_many(Database *db, UserList *list);

bool User_read(Database *db, char* DNI, User *obj);
bool User_read_all(Database *db, UserList *out);
bool User_update(Database *db, User *obj);
bool User_update_many(Database *db, UserList *list);

bool User_delete(Database *db, char* DNI);
bool User_delete_many(Database *db, UserList *list);

bool User_query_unsafe(Database *db, const char *where, UserList *out);

void User_free(User *obj);
void UserList_free(UserList *list);
