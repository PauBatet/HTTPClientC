#include "unity/unity.h"
#include "../.cache/models/Group.h"
#include "../.engine/Database/Database.h"
#include "mock_config.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#define ASSERT_TRUE_MSG(expr, msg, ...) do {        \
    if (!(expr)) {                                 \
        char _buf[256];                            \
        snprintf(_buf, sizeof(_buf), msg, ##__VA_ARGS__); \
        TEST_FAIL_MESSAGE(_buf);                   \
    }                                              \
} while (0)

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
    
    TEST_ASSERT_TRUE(Group_query_unsafe(test_db, query_buf, &search_results));
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
    Group_query_unsafe(test_db, where_clause, &pre_check);
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
    TEST_ASSERT_TRUE(Group_query_unsafe(test_db, where_clause, &query_res));
    
    // We expect exactly 10 + whatever was there (which should be 10 if overlapping_data is 0)
    TEST_ASSERT_EQUAL_INT(10 + overlapping_data, query_res.count);

    // 4. Cleanup ONLY this batch
    TEST_ASSERT_TRUE(Group_delete_many(test_db, &query_res));

    GroupList_free(&batch);
    GroupList_free(&query_res);
}

// 3. Test race-condition behavior (lost update simulation)
void test_Group_Race_Condition_Simulation(void) {
    // Create base group
    Group g = { .name = strdup("RaceBase"), .num_members = 10 };
    TEST_ASSERT_TRUE(Group_create(test_db, &g));

    // Fetch ID
    GroupList list = {0};
    TEST_ASSERT_TRUE(Group_query_unsafe(test_db, "\"name\" = 'RaceBase'", &list));
    TEST_ASSERT_EQUAL_INT(1, list.count);
    int id = list.items[0].id;
    GroupList_free(&list);

    // Simulate two readers
    Group a = {0};
    Group b = {0};

    TEST_ASSERT_TRUE(Group_read(test_db, id, &a));
    TEST_ASSERT_TRUE(Group_read(test_db, id, &b));

    // Both modify based on same initial state
    a.num_members += 5;  // 15
    b.num_members += 20; // 30

    // First update commits
    TEST_ASSERT_TRUE(Group_update(test_db, &a));

    // Second update commits (overwrites)
    TEST_ASSERT_TRUE(Group_update(test_db, &b));

    // Final state reflects LAST writer
    Group final = {0};
    TEST_ASSERT_TRUE(Group_read(test_db, id, &final));
    TEST_ASSERT_EQUAL_INT(30, final.num_members);

    // Cleanup
    Group_delete(test_db, id);
    Group_free(&g);
    Group_free(&a);
    Group_free(&b);
    Group_free(&final);
}

static double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

// 4. Test real transaction locking with FOR UPDATE
void test_Group_Transaction_Locking(void) {
    // Create a unique group for this test run
    char namebuf[64];
    snprintf(namebuf, sizeof(namebuf), "LockTest_%ld_%d",
             (long)time(NULL), getpid());

    Group g = { .name = strdup(namebuf), .num_members = 1 };
    TEST_ASSERT_TRUE(Group_create(test_db, &g));

    // Fetch the ID of the group we just created
    GroupList list = {0};
    char where[128];
    snprintf(where, sizeof(where), "\"name\" = '%s'", namebuf);
    TEST_ASSERT_TRUE(Group_query_unsafe(test_db, where, &list));
    TEST_ASSERT_EQUAL_INT(1, list.count);
    int id = list.items[0].id;
    GroupList_free(&list);

#if defined(DB_BACKEND_POSTGRES)
    // ---------------- Postgres ----------------
    int pipefd[2];
    TEST_ASSERT_TRUE(pipe(pipefd) == 0);

    pid_t pid = fork();
    TEST_ASSERT_TRUE(pid >= 0);

    if (pid == 0) {
        // CHILD — hold row lock
        close(pipefd[0]);
        Database *db2;
        db_open(&db2);

        TEST_ASSERT_TRUE(db_exec(db2, "BEGIN"));

        Group locked = {0};
        TEST_ASSERT_TRUE(Group_read_for_update(db2, id, &locked));

        write(pipefd[1], "1", 1);
        sleep(2);

        locked.num_members += 10;
        TEST_ASSERT_TRUE(Group_update(db2, &locked));
        TEST_ASSERT_TRUE(db_exec(db2, "COMMIT"));

        Group_free(&locked);
        db_close(db2);
        _exit(0);
    }

    // PARENT
    close(pipefd[1]);
    char c;
    read(pipefd[0], &c, 1);

    TEST_ASSERT_TRUE(db_exec(test_db, "BEGIN"));
    Group contender = {0};

    double t0 = now_sec();
    bool ok = Group_read_for_update(test_db, id, &contender);
    double elapsed = now_sec() - t0;

    ASSERT_TRUE_MSG(ok, "Group_read_for_update failed (elapsed=%.3f)", elapsed);
    ASSERT_TRUE_MSG(elapsed >= 1.5,
        "Postgres did NOT block on FOR UPDATE (elapsed=%.3fs)", elapsed);

    db_exec(test_db, "ROLLBACK");
    int status;
    waitpid(pid, &status, 0);

#elif defined(DB_BACKEND_SQLITE)
    // ---------------- SQLite ----------------
    int pipefd[2];
    TEST_ASSERT_TRUE(pipe(pipefd) == 0);

    pid_t pid = fork();
    TEST_ASSERT_TRUE(pid >= 0);

    if (pid == 0) {
        // CHILD: acquire write lock
        close(pipefd[0]);

        Database *db2;
        db_open(&db2);

        TEST_ASSERT_TRUE(db_exec(db2, "BEGIN"));

        Group child = {0};
        TEST_ASSERT_TRUE(Group_read(db2, id, &child));
        child.num_members += 10;

        // This UPDATE acquires the DB-wide write lock
        TEST_ASSERT_TRUE(Group_update(db2, &child));

        // signal parent AFTER lock is held
        write(pipefd[1], "1", 1);

        sleep(2); // hold lock

        TEST_ASSERT_TRUE(db_exec(db2, "COMMIT"));

        Group_free(&child);
        db_close(db2);
        _exit(0);
    }

    // ---------------- Parent ----------------
    close(pipefd[1]);
    char dummy;
    read(pipefd[0], &dummy, 1); // wait for child to lock DB

    TEST_ASSERT_TRUE(db_exec(test_db, "BEGIN"));

    Group contender = {0};
    TEST_ASSERT_TRUE(Group_read(test_db, id, &contender));
    contender.num_members += 5;

    double t1 = now_sec();
    bool ok = Group_update(test_db, &contender);
    double elapsed = now_sec() - t1;

    // SQLite MUST fail immediately with DB locked
    TEST_ASSERT_FALSE_MESSAGE(ok,
        "SQLite UPDATE unexpectedly succeeded while DB was locked");

    ASSERT_TRUE_MSG(elapsed < 0.5,
        "SQLite UPDATE blocked instead of failing fast (elapsed=%.3fs)", elapsed);

    db_exec(test_db, "ROLLBACK");

    // CRITICAL: wait for child to release lock
    waitpid(pid, NULL, 0);
#endif

    // Cleanup
    Group_delete(test_db, id);
    Group_free(&g);
    Group_free(&contender);
}


// 5. Test SQL injection is neutralized by prepared statements
void test_Group_SQL_Injection_Neutralized(void) {
    const char *payload = "x'); DROP TABLE \"Group\"; --";

    Group g = {
        .name = strdup(payload),
        .num_members = 123
    };

    // MUST succeed — injection is data, not SQL
    TEST_ASSERT_TRUE(Group_create(test_db, &g));

    // Table must still exist
    GroupList all = {0};
    TEST_ASSERT_TRUE(Group_read_all(test_db, &all));

    bool found = false;
    for (size_t i = 0; i < all.count; i++) {
        if (strcmp(all.items[i].name, payload) == 0) {
            found = true;
            Group_delete(test_db, all.items[i].id);
            break;
        }
    }

    TEST_ASSERT_TRUE_MESSAGE(found,
        "Injection payload was not stored literally");

    GroupList_free(&all);
    Group_free(&g);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_Group_Individual_Operations);
    RUN_TEST(test_Group_Bulk_And_Query_Operations);
    RUN_TEST(test_Group_Race_Condition_Simulation);
    RUN_TEST(test_Group_Transaction_Locking);
    RUN_TEST(test_Group_SQL_Injection_Neutralized);
    return UNITY_END();
}


