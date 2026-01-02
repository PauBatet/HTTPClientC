#include "../Models.h"
#include "../Database/Database.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

Model *model_registry = NULL;

static void init_generated_models_header() {
    const char *path = "GeneratedModels.h";
    FILE *f = fopen(path, "w"); // "w" truncates the file, starting fresh
    if (!f) return;
    fprintf(f, "#pragma once\n\n");
    fclose(f);
}

static void append_to_generated_models_header(const char *model_name) {
    const char *path = "GeneratedModels.h";
    FILE *f = fopen(path, "a"); // Use "a" for simple appending
    if (!f) return;
    fprintf(f, "#include \".cache/models/%s.h\"\n", model_name);
    fclose(f);
}

// Generate CRUD C and H files
static void generate_crud_files(Model *m) {
    char path_h[512], path_c[512];
    mkdir(".cache/models", 0755);

    snprintf(path_h, sizeof(path_h), ".cache/models/%s.h", m->name);
    snprintf(path_c, sizeof(path_c), ".cache/models/%s.c", m->name);

    FILE *fh = fopen(path_h, "w");
    FILE *fc = fopen(path_c, "w");
    if (!fh || !fc) return;

    append_to_generated_models_header(m->name);

    // --- H file ---
    fprintf(fh,
        "#pragma once\n"
        "#include \"../../.engine/Database/Database.h\"\n"
        "#include <stddef.h>\n\n"
        "typedef struct {\n"
    );

    for (int i = 0; i < m->num_fields; i++) {
        const char *ctype = "char*";
        if (m->fields[i].type == TYPE_INT || m->fields[i].type == TYPE_BOOL) ctype = "int";
        else if (m->fields[i].type == TYPE_FLOAT) ctype = "float";
        fprintf(fh, "    %s %s;\n", ctype, m->fields[i].name);
    }

    fprintf(fh,
        "} %s;\n\n"
        "typedef struct {\n"
        "    %s *items;\n"
        "    size_t count;\n"
        "} %sList;\n\n",
        m->name,
        m->name,
        m->name
    );

    // Determine primary key C type for function prototypes
    const char *pk_ctype = "char*";
    if (m->fields[0].type == TYPE_INT || m->fields[0].type == TYPE_BOOL) pk_ctype = "int";
    else if (m->fields[0].type == TYPE_FLOAT) pk_ctype = "float";

    fprintf(fh,
        "bool %s_create(Database *db, %s *obj);\n"
        "bool %s_create_many(Database *db, %sList *list);\n\n"

        "bool %s_read(Database *db, %s %s, %s *obj);\n"

        "bool %s_read_all(Database *db, %sList *out);\n"

        "bool %s_update(Database *db, %s *obj);\n"
        "bool %s_update_many(Database *db, %sList *list);\n\n"

        "bool %s_delete(Database *db, %s %s);\n"
        "bool %s_delete_many(Database *db, %sList *list);\n\n"

        "bool %s_query_unsafe(Database *db, const char *where, %sList *out);\n\n"

        "void %s_free(%s *obj);\n"
        "void %sList_free(%sList *list);\n",

        m->name, m->name,                    // create
        m->name, m->name,                    // create_many

        m->name, pk_ctype, m->fields[0].name, m->name, // read

        m->name, m->name,                    // read_all

        m->name, m->name,                    // update
        m->name, m->name,                    // update_many

        m->name, pk_ctype, m->fields[0].name,// delete
        m->name, m->name,                    // delete_many

        m->name, m->name,                    // query_unsafe

        m->name, m->name,                    // free
        m->name, m->name                     // List_free    
    );
    fclose(fh);

    // --- C file ---
    fprintf(fc,
        "#include \"%s\"\n"
        "#include <stdio.h>\n"
        "#include <stdlib.h>\n"
        "#include <string.h>\n"
        "#include \"../../.engine/Database/Database.h\"\n\n",
        path_h
    );

   /* CREATE */
    fprintf(fc,
        "bool %s_create(Database *db, %s *obj) {\n"
        "    const char *params[%d];\n",
        m->name, m->name,
        m->num_fields - (m->has_explicit_pk ? 0 : 1)
    );

    int create_start = m->has_explicit_pk ? 0 : 1;

    for (int i = create_start, p = 0; i < m->num_fields; i++, p++) {
        if (m->fields[i].type == TYPE_INT || m->fields[i].type == TYPE_BOOL) {
            fprintf(fc,
                "    char param_%s[32]; snprintf(param_%s, sizeof(param_%s), \"%%d\", obj->%s);\n"
                "    params[%d] = param_%s;\n",
                m->fields[i].name,
                m->fields[i].name,
                m->fields[i].name,
                m->fields[i].name,
                p,
                m->fields[i].name
            );
        } else if (m->fields[i].type == TYPE_FLOAT) {
            fprintf(fc,
                "    char param_%s[64]; snprintf(param_%s, sizeof(param_%s), \"%%f\", obj->%s);\n"
                "    params[%d] = param_%s;\n",
                m->fields[i].name,
                m->fields[i].name,
                m->fields[i].name,
                m->fields[i].name,
                p,
                m->fields[i].name
            );
        } else {
            fprintf(fc,
                "    params[%d] = obj->%s;\n",
                p, m->fields[i].name
            );
        }
    }

    fprintf(fc,
        "    const char *sql = \"INSERT INTO \\\"%s\\\" (",
        m->name
    );

    for (int i = create_start; i < m->num_fields; i++) {
        fprintf(fc, "\\\"%s\\\"%s",
            m->fields[i].name,
            i < m->num_fields - 1 ? ", " : ""
        );
    }

    fprintf(fc, ") VALUES (");

    for (int i = create_start, p = 1; i < m->num_fields; i++, p++) {
        fprintf(fc, "?%d%s", p, i < m->num_fields - 1 ? ", " : "");
    }

    fprintf(fc,
        ")\";\n"
        "    return db_exec_params(db, sql, %d, params);\n"
        "}\n\n",
        m->num_fields - create_start
    );

    /* CREATE MANY */
    fprintf(fc,
        "bool %s_create_many(Database *db, %sList *list) {\n"
        "    if (!db_exec(db, \"BEGIN\")) return false;\n"
        "    for (size_t i = 0; i < list->count; i++) {\n"
        "        if (!%s_create(db, &list->items[i])) {\n"
        "            db_exec(db, \"ROLLBACK\");\n"
        "            return false;\n"
        "        }\n"
        "    }\n"
        "    return db_exec(db, \"COMMIT\");\n"
        "}\n\n",
        m->name, m->name, m->name
    );

    /* READ */
    fprintf(fc,
        "bool %s_read(Database *db, %s %s, %s *obj) {\n"
        "    %s_free(obj);\n"
        "    memset(obj, 0, sizeof(*obj));\n"
        "    DBResult *r;\n"
        "    const char *params[1];\n",
        m->name,
        pk_ctype,
        m->fields[0].name,
        m->name,
        m->name
    );

    /* PK param */
    if (m->fields[0].type == TYPE_INT || m->fields[0].type == TYPE_BOOL) {
        fprintf(fc,
            "    char pk_buf[32]; snprintf(pk_buf, sizeof(pk_buf), \"%%d\", %s);\n"
            "    params[0] = pk_buf;\n",
            m->fields[0].name
        );
    } else if (m->fields[0].type == TYPE_FLOAT) {
        fprintf(fc,
            "    char pk_buf[64]; snprintf(pk_buf, sizeof(pk_buf), \"%%f\", %s);\n"
            "    params[0] = pk_buf;\n",
            m->fields[0].name
        );
    } else {
        fprintf(fc,
            "    params[0] = %s;\n",
            m->fields[0].name
        );
    }

    fprintf(fc,
        "    const char *sql = \"SELECT "
    );

    for (int i = 0; i < m->num_fields; i++) {
        fprintf(fc, "\\\"%s\\\"%s",
            m->fields[i].name,
            i < m->num_fields - 1 ? ", " : ""
        );
    }

    fprintf(fc,
        " FROM \\\"%s\\\" WHERE \\\"%s\\\" = ?1\";\n"
        "    if (!db_query_params(db, sql, 1, params, &r)) return false;\n"
        "    bool ok = false;\n"
        "    if (db_result_next(r)) {\n",
        m->name,
        m->fields[0].name
    );

    for (int i = 0; i < m->num_fields; i++) {
        if (m->fields[i].type == TYPE_INT || m->fields[i].type == TYPE_BOOL)
            fprintf(fc, "        obj->%s = db_result_int(r, %d);\n", m->fields[i].name, i);
        else if (m->fields[i].type == TYPE_FLOAT)
            fprintf(fc, "        obj->%s = db_result_double(r, %d);\n", m->fields[i].name, i);
        else
            fprintf(fc, "        obj->%s = db_result_string(r, %d);\n", m->fields[i].name, i);
    }

    fprintf(fc,
        "        ok = true;\n"
        "    }\n"
        "    db_result_free(r);\n"
        "    return ok;\n"
        "}\n\n"
    );

    /* READ_ALL */
    fprintf(fc,
        "bool %s_read_all(Database *db, %sList *out) {\n"
        "    DBResult *r;\n"
        "    const char *sql = \"SELECT ",
        m->name, m->name
    );

    for (int i = 0; i < m->num_fields; i++) {
        fprintf(fc, "\\\"%s\\\"%s", m->fields[i].name, i < m->num_fields - 1 ? ", " : "");
    }

    fprintf(fc,
        " FROM \\\"%s\\\"\";\n"
        "    if (!db_query(db, sql, &r)) return false;\n"
        "    size_t cap = 8;\n"
        "    out->count = 0;\n"
        "    out->items = calloc(cap, sizeof(%s));\n"
        "    while (db_result_next(r)) {\n"
        "        if (out->count == cap) {\n"
        "            cap *= 2;\n"
        "            void *tmp = realloc(out->items, sizeof(%s) * cap);\n"
        "            if (!tmp) {\n"
        "                for (size_t j = 0; j < out->count; j++) %s_free(&out->items[j]);\n"
        "                free(out->items);\n"
        "                out->items = NULL;\n"
        "                out->count = 0;\n"
        "                db_result_free(r);\n"
        "                return false;\n"
        "            }\n"
        "            out->items = tmp;\n"
        "        }\n"
        "        %s *obj = &out->items[out->count++];\n",
        m->name, m->name, m->name, m->name, m->name
    );

    for (int i = 0; i < m->num_fields; i++) {
        if (m->fields[i].type == TYPE_INT || m->fields[i].type == TYPE_BOOL)
            fprintf(fc, "        obj->%s = db_result_int(r, %d);\n", m->fields[i].name, i);
        else if (m->fields[i].type == TYPE_FLOAT)
            fprintf(fc, "        obj->%s = db_result_double(r, %d);\n", m->fields[i].name, i);
        else
            fprintf(fc, "        obj->%s = db_result_string(r, %d);\n", m->fields[i].name, i);
    }

    fprintf(fc,
        "    }\n"
        "    db_result_free(r);\n"
        "    return true;\n"
        "}\n\n"
    );

    /* QUERY */
    fprintf(fc,
        "/* WARNING: this function is SQL-injection prone. */\n"
        "bool %s_query_unsafe(Database *db, const char *where, %sList *out) {\n"
        "    char sql[1024];\n"
        "    DBResult *r;\n"
        "    snprintf(sql, sizeof(sql), \"SELECT ",
        m->name, m->name
    );

    for (int i = 0; i < m->num_fields; i++) {
        fprintf(fc, "\\\"%s\\\"%s", m->fields[i].name, i < m->num_fields - 1 ? ", " : "");
    }

    fprintf(fc,
        " FROM \\\"%s\\\" WHERE %%s\", where);\n"
        "    if (!db_query(db, sql, &r)) return false;\n"
        "    size_t cap = 8;\n"
        "    out->count = 0;\n"
        "    out->items = calloc(cap, sizeof(%s));\n"
        "    while (db_result_next(r)) {\n"
        "        if (out->count == cap) {\n"
        "            cap *= 2;\n"
        "            void *tmp = realloc(out->items, sizeof(%s) * cap);\n"
        "            if (!tmp) {\n"
        "                for (size_t j = 0; j < out->count; j++) %s_free(&out->items[j]);\n"
        "                free(out->items);\n"
        "                out->items = NULL;\n"
        "                out->count = 0;\n"
        "                db_result_free(r);\n"
        "                return false;\n"
        "            }\n"
        "            out->items = tmp;\n"
        "        }\n"
        "        %s *obj = &out->items[out->count++];\n",
        m->name, m->name, m->name, m->name, m->name
    );

    for (int i = 0; i < m->num_fields; i++) {
        if (m->fields[i].type == TYPE_INT || m->fields[i].type == TYPE_BOOL)
            fprintf(fc, "        obj->%s = db_result_int(r, %d);\n", m->fields[i].name, i);
        else if (m->fields[i].type == TYPE_FLOAT)
            fprintf(fc, "        obj->%s = db_result_double(r, %d);\n", m->fields[i].name, i);
        else
            fprintf(fc, "        obj->%s = db_result_string(r, %d);\n", m->fields[i].name, i);
    }

    fprintf(fc,
        "    }\n"
        "    db_result_free(r);\n"
        "    return true;\n"
        "}\n\n"
    );

    /* CLEANUP HELPERS */
    fprintf(fc,
        "void %s_free(%s *obj) {\n",
        m->name, m->name
    );
    fprintf(fc, "    if (!obj) return;\n");

    for (int i = 0; i < m->num_fields; i++) {
        if (m->fields[i].type == TYPE_STRING)
            fprintf(fc, "    free(obj->%s);\n", m->fields[i].name);
    }

    fprintf(fc, "}\n\n");

    fprintf(fc,
        "void %sList_free(%sList *list) {\n"
        "    for (size_t i = 0; i < list->count; i++) {\n"
        "        %s_free(&list->items[i]);\n"
        "    }\n"
        "    free(list->items);\n"
        "    list->items = NULL;\n"
        "    list->count = 0;\n"
        "}\n",
        m->name, m->name, m->name
    );

    /* UPDATE */
    fprintf(fc,
        "bool %s_update(Database *db, %s *obj) {\n"
        "    const char *params[%d];\n",
        m->name, m->name, m->num_fields
    );

    for (int i = 1, p = 0; i < m->num_fields; i++, p++) {
        if (m->fields[i].type == TYPE_INT || m->fields[i].type == TYPE_BOOL) {
            fprintf(fc,
                "    char param_%s[32]; snprintf(param_%s, sizeof(param_%s), \"%%d\", obj->%s);\n"
                "    params[%d] = param_%s;\n",
                m->fields[i].name,
                m->fields[i].name,
                m->fields[i].name,
                m->fields[i].name,
                p,
                m->fields[i].name
            );
        } else if (m->fields[i].type == TYPE_FLOAT) {
            fprintf(fc,
                "    char param_%s[64]; snprintf(param_%s, sizeof(param_%s), \"%%f\", obj->%s);\n"
                "    params[%d] = param_%s;\n",
                m->fields[i].name,
                m->fields[i].name,
                m->fields[i].name,
                m->fields[i].name,
                p,
                m->fields[i].name
            );
        } else {
            fprintf(fc, "    params[%d] = obj->%s;\n", p, m->fields[i].name);
        }
    }

    /* PK param */
    if (m->fields[0].type == TYPE_INT || m->fields[0].type == TYPE_BOOL) {
        fprintf(fc,
            "    char pk_buf[32]; snprintf(pk_buf, sizeof(pk_buf), \"%%d\", obj->%s);\n"
            "    params[%d] = pk_buf;\n",
            m->fields[0].name,
            m->num_fields - 1
        );
    } else if (m->fields[0].type == TYPE_FLOAT) {
        fprintf(fc,
            "    char pk_buf[64]; snprintf(pk_buf, sizeof(pk_buf), \"%%f\", obj->%s);\n"
            "    params[%d] = pk_buf;\n",
            m->fields[0].name,
            m->num_fields - 1
        );
    } else {
        fprintf(fc,
            "    params[%d] = obj->%s;\n",
            m->num_fields - 1,
            m->fields[0].name
        );
    }

    fprintf(fc,
        "    const char *sql = \"UPDATE \\\"%s\\\" SET ",
        m->name
    );

    for (int i = 1, p = 1; i < m->num_fields; i++, p++) {
        fprintf(fc, "\\\"%s\\\" = ?%d%s",
                m->fields[i].name,
                p,
                i < m->num_fields - 1 ? ", " : "");
    }

    fprintf(fc,
        " WHERE \\\"%s\\\" = ?%d\";\n"
        "    return db_exec_params(db, sql, %d, params);\n"
        "}\n\n",
        m->fields[0].name,
        m->num_fields,
        m->num_fields
    );

    /* UPDATE MANY */
    fprintf(fc,
        "bool %s_update_many(Database *db, %sList *list) {\n"
        "    if (!db_exec(db, \"BEGIN\")) return false;\n"
        "    for (size_t i = 0; i < list->count; i++) {\n"
        "        if (!%s_update(db, &list->items[i])) {\n"
        "            db_exec(db, \"ROLLBACK\");\n"
        "            return false;\n"
        "        }\n"
        "    }\n"
        "    return db_exec(db, \"COMMIT\");\n"
        "}\n\n",
        m->name, m->name, m->name
    );

    /* DELETE */
    fprintf(fc,
        "bool %s_delete(Database *db, %s %s) {\n"
        "    const char *params[1];\n",
        m->name, pk_ctype, m->fields[0].name
    );

    if (m->fields[0].type == TYPE_INT || m->fields[0].type == TYPE_BOOL) {
        fprintf(fc,
            "    char pk_buf[32]; snprintf(pk_buf, sizeof(pk_buf), \"%%d\", %s);\n"
            "    params[0] = pk_buf;\n",
            m->fields[0].name
        );
    } else if (m->fields[0].type == TYPE_FLOAT) {
        fprintf(fc,
            "    char pk_buf[64]; snprintf(pk_buf, sizeof(pk_buf), \"%%f\", %s);\n"
            "    params[0] = pk_buf;\n",
            m->fields[0].name
        );
    } else {
        fprintf(fc, "    params[0] = %s;\n", m->fields[0].name);
    }

    fprintf(fc,
        "    const char *sql = \"DELETE FROM \\\"%s\\\" WHERE \\\"%s\\\" = ?1\";\n"
        "    return db_exec_params(db, sql, 1, params);\n"
        "}\n\n",
        m->name, m->fields[0].name
    );

    /* DELETE MANY */
    fprintf(fc,
        "bool %s_delete_many(Database *db, %sList *list) {\n"
        "    if (!db_exec(db, \"BEGIN\")) return false;\n"
        "    for (size_t i = 0; i < list->count; i++) {\n"
        "        if (!%s_delete(db, list->items[i].%s)) {\n"
        "            db_exec(db, \"ROLLBACK\");\n"
        "            return false;\n"
        "        }\n"
        "    }\n"
        "    return db_exec(db, \"COMMIT\");\n"
        "}\n\n",
        m->name,
        m->name,
        m->name,
        m->fields[0].name
    );
    fclose(fc);
}

// Add a model to the registry
void model(
    const char *name,
    Field *primary_key,       // NEW parameter
    Field *fields,
    int num_fields,
    ForeignKey *foreign_keys,
    int num_foreign_keys
) {
    Model *m = calloc(1, sizeof(Model));
    if (!m) { perror("malloc"); exit(1); }

    m->name = strdup(name);

    int auto_id = (primary_key == NULL);
    m->num_fields = num_fields + 1;
    m->fields = malloc(sizeof(Field) * m->num_fields);

    m->has_explicit_pk = (primary_key != NULL);

    if (auto_id) {
        m->fields[0].type = TYPE_INT;
        m->fields[0].name = strdup("id");
    } else {
        m->fields[0].type = primary_key->type;
        m->fields[0].name = strdup(primary_key->name);
    }

    for (int i = 0; i < num_fields; i++) {
        m->fields[i + 1].type = fields[i].type;
        m->fields[i + 1].name = strdup(fields[i].name);
    }

    // Foreign keys
    m->num_foreign_keys = num_foreign_keys;
    if (num_foreign_keys > 0 && foreign_keys) {
        m->foreign_keys = malloc(sizeof(ForeignKey) * num_foreign_keys);
        for (int i = 0; i < num_foreign_keys; i++) {
            m->foreign_keys[i].field_name = strdup(foreign_keys[i].field_name);
            m->foreign_keys[i].ref_table  = strdup(foreign_keys[i].ref_table);
            m->foreign_keys[i].ref_field  = strdup(foreign_keys[i].ref_field);
        }
    } else {
        m->foreign_keys = NULL;
    }

    m->next = model_registry;
    model_registry = m;

    generate_crud_files(m);
}

// Generate tables in the DB
void generate_model_tables(Database *db) {
    Model *m = model_registry;
    while (m) {
        char sql[4096] = {0};
        strcat(sql, "CREATE TABLE IF NOT EXISTS ");
        strcat(sql, "\"");
        strcat(sql, m->name);
        strcat(sql, "\"");

        strcat(sql, " (");

        for (int i = 0; i < m->num_fields; i++) {
            char buffer[256];
            const char *type_str = "TEXT";
            switch (m->fields[i].type) {
                case TYPE_INT: type_str = "INTEGER"; break;
                case TYPE_FLOAT: type_str = "REAL"; break;
                case TYPE_STRING: type_str = "TEXT"; break;
                case TYPE_BOOL: type_str = "INTEGER"; break;
            }

            if (i == 0) {
                // AUTO-INCREMENT only if it's the auto-generated "id"
                if (strcmp(m->fields[i].name, "id") == 0) {
                    snprintf(buffer, sizeof(buffer), "%s %s PRIMARY KEY AUTOINCREMENT%s",
                            m->fields[i].name, type_str,
                            m->num_fields - 1 > 0 || m->num_foreign_keys > 0 ? ", " : "");
                } else {
                    snprintf(buffer, sizeof(buffer), "%s %s PRIMARY KEY%s",
                            m->fields[i].name, type_str,
                            m->num_fields - 1 > 0 || m->num_foreign_keys > 0 ? ", " : "");
                }
            } else {
                snprintf(buffer, sizeof(buffer), "%s %s%s",
                        m->fields[i].name, type_str,
                        i < m->num_fields - 1 || m->num_foreign_keys > 0 ? ", " : "");
            }

            strcat(sql, buffer);
        }

        for (int i = 0; i < m->num_foreign_keys; i++) {
            char buffer[256];
            snprintf(buffer, sizeof(buffer), "FOREIGN KEY(%s) REFERENCES \"%s\"(%s)%s",
                     m->foreign_keys[i].field_name,
                     m->foreign_keys[i].ref_table,
                     m->foreign_keys[i].ref_field,
                     i < m->num_foreign_keys - 1 ? ", " : "");
            strcat(sql, buffer);
        }

        strcat(sql, ");");

        if (!db_exec(db, sql)) {
            printf("Failed to create table %s\n", m->name);
        } else {
            printf("Table %s created or already exists.\n", m->name);
        }

        m = m->next;
    }
}

static ModelDefFn *defs = NULL;
static size_t def_count = 0;

void register_model_def(ModelDefFn fn) {
    defs = realloc(defs, sizeof(ModelDefFn) * (def_count + 1));
    defs[def_count++] = fn;
}

void run_model_definitions(void) {
    for (size_t i = 0; i < def_count; i++) {
        defs[i]();
    }
}

int main() {
    load_config_from_env();
    init_generated_models_header();
    Database *db;
    if (!db_open(&db)) {
        printf("Failed to open database\n");
        return 1;
    }

    run_model_definitions();
    generate_model_tables(db);

    db_close(db);
    return 0;
}

