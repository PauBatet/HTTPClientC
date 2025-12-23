#include "../.engine/Models/Models.h"
#include <stddef.h>

void register_group_model(void) {
    Field group_fields[] = {
        {"name", TYPE_STRING},
        {"num_members", TYPE_INT}
    };

    model("Group", NULL, group_fields, 2, NULL, 0);
}

__attribute__((constructor))
static void register_root_models(void) {
    register_model_def(register_group_model);
}
