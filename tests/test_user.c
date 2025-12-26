#include "unity/unity.h"
#include "../.cache/models/User.h"
#include "../.cache/models/Group.h"
#include "../.engine/Database/Database.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

Database *test_db;
int shared_group_id = 0;

void setUp(void) {
    db_open(&test_db);
    
    // 1. Create a parent group for the users to belong to
    char g_name[64];
    sprintf(g_name, "UserTestGroup_%ld", (long)time(NULL));
    Group g = { .name = strdup(g_name), .num_members = 0 };
    Group_create(test_db, &g);

    // 2. Find the ID assigned to that group
    GroupList gl = {0};
    char q[128];
    sprintf(q, "\"name\" = '%s'", g_name);
    Group_query(test_db, q, &gl);
    if (gl.count > 0) shared_group_id = gl.items[0].id;

    Group_free(&g);
    GroupList_free(&gl);
}

void tearDown(void) {
    db_close(test_db);
}

void test_User_Individual_CRUD(void) {
    TEST_ASSERT_NOT_EQUAL_MESSAGE(0, shared_group_id, "Setup failed to create parent group");

    char unique_dni[16];
    sprintf(unique_dni, "DNI_%d", rand() % 100000);

    // 1. CREATE
    User u = {
        .DNI = strdup(unique_dni),
        .name = strdup("Test User"),
        .age = 25,
        .email = strdup("test@example.com"),
        .group_id = shared_group_id
    };
    TEST_ASSERT_TRUE_MESSAGE(User_create(test_db, &u), "User_create failed");

    // 2. READ
    User fetched = {0};
    TEST_ASSERT_TRUE(User_read(test_db, unique_dni, &fetched));
    TEST_ASSERT_EQUAL_STRING("Test User", fetched.name);
    TEST_ASSERT_EQUAL_INT(shared_group_id, fetched.group_id);

    // 3. UPDATE
    free(fetched.name);
    fetched.name = strdup("Updated Name");
    TEST_ASSERT_TRUE(User_update(test_db, &fetched));

    // 4. DELETE
    TEST_ASSERT_TRUE(User_delete(test_db, unique_dni));

    User_free(&u);
    User_free(&fetched);
}

void test_User_Bulk_Operations(void) {
    char batch_tag[32];
    sprintf(batch_tag, "B%d", rand() % 1000);

    // 1. CREATE MANY
    UserList list = {0};
    list.count = 5;
    list.items = calloc(list.count, sizeof(User));
    for (int i = 0; i < 5; i++) {
        char dni[64], email[64];
        sprintf(dni, "DNI_%s_%d", batch_tag, i);
        sprintf(email, "user_%d@%s.com", i, batch_tag);
        list.items[i].DNI = strdup(dni);
        list.items[i].name = strdup("Bulk User");
        list.items[i].age = 20 + i;
        list.items[i].email = strdup(email);
        list.items[i].group_id = shared_group_id;
    }
    TEST_ASSERT_TRUE(User_create_many(test_db, &list));

    // 2. QUERY
    UserList results = {0};
    char where[128];
    sprintf(where, "\"DNI\" LIKE 'DNI_%s_%%'", batch_tag);
    TEST_ASSERT_TRUE(User_query(test_db, where, &results));
    TEST_ASSERT_EQUAL_INT(5, results.count);

    // 3. DELETE MANY
    TEST_ASSERT_TRUE(User_delete_many(test_db, &results));

    UserList_free(&list);
    UserList_free(&results);
}

int main(void) {
    srand(time(NULL));
    UNITY_BEGIN();
    RUN_TEST(test_User_Individual_CRUD);
    RUN_TEST(test_User_Bulk_Operations);
    return UNITY_END();
}
