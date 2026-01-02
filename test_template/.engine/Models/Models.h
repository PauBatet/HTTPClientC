#pragma once
#include "../Database/Database.h"
#include <stdbool.h>

// Field types
typedef enum {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_STRING,
    TYPE_BOOL
} FieldType;

// Field definition
typedef struct {
    const char *name;
    FieldType type;
} Field;

// Foreign key definition
typedef struct {
    const char *field_name;
    const char *ref_table;
    const char *ref_field;
} ForeignKey;

// Hook function types
typedef void (*HookFunc)(Database *db, int id);

// Model registry entry
typedef struct Model {
    const char *name;
    Field *primary_key;
    Field *fields;
    int num_fields;

    ForeignKey *foreign_keys;
    int num_foreign_keys;
    int has_explicit_pk;

    struct Model *next;
} Model;

// Registry head
extern Model *model_registry;

// Function to declare a model
void model(
    const char *name,
    Field *primary_key,
    Field *fields,
    int num_fields,
    ForeignKey *foreign_keys,
    int num_foreign_keys
);

// Function to generate tables for all registered models
void generate_model_tables(Database *db);

typedef void (*ModelDefFn)(void);

void register_model_def(ModelDefFn fn);
void run_model_definitions(void);
