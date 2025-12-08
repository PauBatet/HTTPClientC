#include "Models.h"
#include "../../models.c"
#include "../Database/Database.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

Model *model_registry = NULL;

static void append_to_generated_models_header(const char *model_name) {
    const char *path = "GeneratedModels.h";

    FILE *f = fopen(path, "a+");
    if (!f) return;

    // Write header guard only once
    fseek(f, 0, SEEK_END);
    if (ftell(f) == 0) {
        fprintf(f, "#pragma once\n\n");
    }

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
        "#include \"../../.engine/Database/Database.h\"\n\n"
        "typedef struct {\n"
    );

    for (int i = 0; i < m->num_fields; i++) {
        const char *ctype = "char*";
        if (m->fields[i].type == TYPE_INT || m->fields[i].type == TYPE_BOOL) ctype = "int";
        else if (m->fields[i].type == TYPE_FLOAT) ctype = "float";
        fprintf(fh, "    %s %s;\n", ctype, m->fields[i].name);
    }

    fprintf(fh, "} %s;\n\n", m->name);

    // Determine primary key C type for function prototypes
    const char *pk_ctype = "char*";
    if (m->fields[0].type == TYPE_INT || m->fields[0].type == TYPE_BOOL) pk_ctype = "int";
    else if (m->fields[0].type == TYPE_FLOAT) pk_ctype = "float";

    fprintf(fh,
        "bool %s_create(Database *db, %s *obj);\n"
        "bool %s_read(Database *db, %s %s, %s *obj);\n"
        "bool %s_update(Database *db, %s *obj);\n"
        "bool %s_delete(Database *db, %s %s);\n",
        m->name,
        m->name,
        m->name, pk_ctype, m->fields[0].name, m->name,
        m->name, m->name,
        m->name, pk_ctype, m->fields[0].name
    );

    fclose(fh);

    // --- C file ---
    fprintf(fc, "#include \"%s\"\n#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n#include \"sqlite3.h\"\n\n", path_h);

    // CREATE
    fprintf(fc,
        "bool %s_create(Database *db, %s *obj) {\n"
        "    char sql[1024];\n"
        "    snprintf(sql, sizeof(sql), \"INSERT INTO \\\"%s\\\" (",
        m->name, m->name, m->name
    );

    for (int i = 0; i < m->num_fields; i++) {
        fprintf(fc, "%s%s", m->fields[i].name, i < m->num_fields - 1 ? ", " : "");
    }

    fprintf(fc, ") VALUES (");

    for (int i = 0; i < m->num_fields; i++) {
        if (m->fields[i].type == TYPE_INT || m->fields[i].type == TYPE_BOOL) fprintf(fc, "%%d");
        else if (m->fields[i].type == TYPE_FLOAT) fprintf(fc, "%%f");
        else fprintf(fc, "'%%s'");
        if (i < m->num_fields - 1) fprintf(fc, ", ");
    }

    fprintf(fc, ")\"");

    for (int i = 0; i < m->num_fields; i++) {
        fprintf(fc, ", obj->%s", m->fields[i].name);
    }

    fprintf(fc, ");\n    return db_exec(db, sql);\n}\n\n");

    // READ
    fprintf(fc,
        "bool %s_read(Database *db, %s %s, %s *obj) {\n"
        "    void *stmt;\n"
        "    char sql[512];\n"
        "    snprintf(sql, sizeof(sql), \"SELECT ",
        m->name, pk_ctype, m->fields[0].name, m->name
    );

    for (int i = 0; i < m->num_fields; i++) {
        fprintf(fc, "%s%s", m->fields[i].name, i < m->num_fields - 1 ? ", " : "");
    }

    fprintf(fc, " FROM \\\"%s\\\" WHERE %s = ", m->name, m->fields[0].name);

    // Format based on primary key type
    if (m->fields[0].type == TYPE_INT || m->fields[0].type == TYPE_BOOL) fprintf(fc, "%%d");
    else if (m->fields[0].type == TYPE_FLOAT) fprintf(fc, "%%f");
    else fprintf(fc, "'%%s'");

    fprintf(fc, "\", %s);\n", m->fields[0].name);

    fprintf(fc, "    if (!db_prepare(db, sql, &stmt)) return false;\n"
                "    bool ok = false;\n"
                "    if (db_step(stmt)) {\n");

    for (int i = 0; i < m->num_fields; i++) {
        if (m->fields[i].type == TYPE_INT || m->fields[i].type == TYPE_BOOL)
            fprintf(fc, "        obj->%s = sqlite3_column_int(stmt, %d);\n", m->fields[i].name, i);
        else if (m->fields[i].type == TYPE_FLOAT)
            fprintf(fc, "        obj->%s = sqlite3_column_double(stmt, %d);\n", m->fields[i].name, i);
        else
            fprintf(fc, "        obj->%s = strdup((const char*)sqlite3_column_text(stmt, %d));\n", m->fields[i].name, i);
    }

    fprintf(fc, "        ok = true;\n    }\n    db_finalize(stmt);\n    return ok;\n}\n\n");

    // UPDATE
    fprintf(fc,
        "bool %s_update(Database *db, %s *obj) {\n"
        "    char sql[1024];\n"
        "    snprintf(sql, sizeof(sql), \"UPDATE \\\"%s\\\" SET ",
        m->name, m->name, m->name
    );

    for (int i = 0; i < m->num_fields; i++) {
        if (i == 0) continue; // skip primary key in SET
        fprintf(fc, "%s=", m->fields[i].name);
        if (m->fields[i].type == TYPE_INT || m->fields[i].type == TYPE_BOOL) fprintf(fc, "%%d");
        else if (m->fields[i].type == TYPE_FLOAT) fprintf(fc, "%%f");
        else fprintf(fc, "'%%s'");
        if (i < m->num_fields - 1) fprintf(fc, ", ");
    }

    fprintf(fc, " WHERE %s = ", m->fields[0].name);
    if (m->fields[0].type == TYPE_INT || m->fields[0].type == TYPE_BOOL) fprintf(fc, "%%d");
    else if (m->fields[0].type == TYPE_FLOAT) fprintf(fc, "%%f");
    else fprintf(fc, "'%%s'");
    fprintf(fc, "\", ");
    for (int i = 1; i < m->num_fields; i++) {
        fprintf(fc, "obj->%s", m->fields[i].name);
        if (i < m->num_fields - 1) fprintf(fc, ", ");
    }
    fprintf(fc, ", obj->%s);\n", m->fields[0].name);
    fprintf(fc, "    return db_exec(db, sql);\n}\n\n");

    // DELETE
    fprintf(fc,
        "bool %s_delete(Database *db, %s %s) {\n"
        "    char sql[256];\n"
        "    snprintf(sql, sizeof(sql), \"DELETE FROM \\\"%s\\\" WHERE %s = ",
        m->name, pk_ctype, m->fields[0].name, m->name, m->fields[0].name
    );

    if (m->fields[0].type == TYPE_INT || m->fields[0].type == TYPE_BOOL) fprintf(fc, "%%d");
    else if (m->fields[0].type == TYPE_FLOAT) fprintf(fc, "%%f");
    else fprintf(fc, "'%%s'");
    fprintf(fc, "\", %s);\n", m->fields[0].name);

    fprintf(fc, "    return db_exec(db, sql);\n}\n");

    fclose(fc);
}

// Add a model to the registry
void model(
    const char *name,
    Field *primary_key,       // NEW parameter
    Field *fields,
    int num_fields,
    ForeignKey *foreign_keys,
    int num_foreign_keys,
    HookFunc on_create,
    HookFunc on_read,
    HookFunc on_update,
    HookFunc on_delete
) {
    Model *m = calloc(1, sizeof(Model));
    if (!m) { perror("malloc"); exit(1); }

    m->name = strdup(name);

    int auto_id = (primary_key == NULL);
    m->num_fields = num_fields + 1;
    m->fields = malloc(sizeof(Field) * m->num_fields);


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

    m->on_create = on_create;
    m->on_read   = on_read;
    m->on_update = on_update;
    m->on_delete = on_delete;

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

int main() {
    Database *db;
    if (!db_open(&db, "app.db")) {
        printf("Failed to open database\n");
        return 1;
    }

    define_models();
    generate_model_tables(db);

    db_close(db);
    return 0;
}

