#include "../.engine/Models/Models.h"
#include <stddef.h>

// This replaces the logic in your real models.c for testing purposes
void register_test_models(void) {
    Field group_fields[] = {
        {"name", TYPE_STRING},
        {"num_members", TYPE_INT}
    };

    Field user_fields[] = {
        {"name", TYPE_STRING},
        {"age", TYPE_INT},
        {"email", TYPE_STRING},
        {"group_id", TYPE_INT}
    };

    ForeignKey user_fk[] = {
        {"group_id", "Group", "id"}
    };

    Field pk_field = { "DNI", TYPE_STRING };

    // Register User with custom PK to test quoting and type mapping
    model("User", &pk_field, user_fields, 4, user_fk, 1);
    // Register Group with default ID
    model("Group", NULL, group_fields, 2, NULL, 0);
}

__attribute__((constructor))
static void register_root_models(void) {
    register_model_def(register_test_models);
}
