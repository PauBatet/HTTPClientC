#include "../.engine/Models/Models.h"
#include <stddef.h>

void register_models(void) {
    
}

__attribute__((constructor))
static void register_root_models(void) {
    register_model_def(register_models);
}
