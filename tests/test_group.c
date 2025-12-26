#include "unity/unity.h"
#include "../.cache/models/Group.h"
#include "../.engine/Database/Database.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

Database *test_db;

void setUp(void) {
    db_open(&test_db);
}

void tearDown(void) {
    db_close(test_db);
}

// 1. Test Individual CRUD
void test_Group_Individual_Operations(void) {
    // 1. Create a unique name to find our record later
    char unique_name[64];
    sprintf(unique_name, "Solo_%ld", (long)time(NULL)); 
    
    Group g = { .name = strdup(unique_name), .num_members = 1 };
    TEST_ASSERT_TRUE(Group_create(test_db, &g));

    // 2. Search for the group we just created using Group_query
    GroupList search_results = {0};
    char query_buf[128];
    sprintf(query_buf, "\"name\" = '%s'", unique_name);
    
    TEST_ASSERT_TRUE(Group_query(test_db, query_buf, &search_results));
    TEST_ASSERT_TRUE_MESSAGE(search_results.count > 0, "Could not find the created group by name");
    
    // 3. Grab the ID assigned by Postgres
    int assigned_id = search_results.items[0].id;

    // 4. Test READ using the ID
    Group fetched = {0};
    TEST_ASSERT_TRUE(Group_read(test_db, assigned_id, &fetched));
    TEST_ASSERT_EQUAL_STRING(unique_name, fetched.name);

    // 5. Test UPDATE
    free(fetched.name);
    fetched.name = strdup("Modified_Solo_Name");
    TEST_ASSERT_TRUE(Group_update(test_db, &fetched));

    // 6. Test DELETE
    TEST_ASSERT_TRUE(Group_delete(test_db, assigned_id));

    // Cleanup
    Group_free(&g);
    Group_free(&fetched);
    GroupList_free(&search_results);
}

// 2. Test Bulk Operations & Realloc Logic
void test_Group_Bulk_And_Query_Operations(void) {
    char batch_tag[32];
    // Use a high-entropy tag
    sprintf(batch_tag, "T%ldR%d", (long)time(NULL), rand() % 1000); 

    // 1. Get count of groups that ALREADY match this tag (should be 0)
    GroupList pre_check = {0};
    char where_clause[128];
    sprintf(where_clause, "\"name\" LIKE 'Batch_%s_%%'", batch_tag);
    Group_query(test_db, where_clause, &pre_check);
    size_t overlapping_data = pre_check.count;
    GroupList_free(&pre_check);

    // 2. CREATE 10 items
    GroupList batch = {0};
    batch.count = 10;
    batch.items = calloc(batch.count, sizeof(Group));
    for (int i = 0; i < 10; i++) {
        char name_buf[64];
        sprintf(name_buf, "Batch_%s_%d", batch_tag, i);
        batch.items[i].name = strdup(name_buf);
        batch.items[i].num_members = i * 5;
    }
    TEST_ASSERT_TRUE(Group_create_many(test_db, &batch));

    // 3. Query ONLY the items from THIS batch
    GroupList query_res = {0};
    TEST_ASSERT_TRUE(Group_query(test_db, where_clause, &query_res));
    
    // We expect exactly 10 + whatever was there (which should be 10 if overlapping_data is 0)
    TEST_ASSERT_EQUAL_INT(10 + overlapping_data, query_res.count);

    // 4. Cleanup ONLY this batch
    TEST_ASSERT_TRUE(Group_delete_many(test_db, &query_res));

    GroupList_free(&batch);
    GroupList_free(&query_res);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_Group_Individual_Operations);
    RUN_TEST(test_Group_Bulk_And_Query_Operations);
    return UNITY_END();
}
