#include <stddef.h>

void define_models() {
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

    model("User", &pk_field, user_fields, 4, user_fk, 1, NULL, NULL, NULL, NULL);

    Field group_fields[] = {
        {"name", TYPE_STRING},
        {"num_members", TYPE_INT}
    };

    model("Group", NULL, group_fields, 2, NULL, 0, NULL, NULL, NULL, NULL);
}

