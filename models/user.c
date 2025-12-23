#include "../.engine/Models/Models.h"
#include <stddef.h>

void register_user_model(void) {
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

    model("User", &pk_field, user_fields, 4, user_fk, 1);
}
__attribute__((constructor))
static void register_root_models(void) {
    register_model_def(register_user_model);
}
