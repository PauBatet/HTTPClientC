#include "HTTPFramework.h"
#include "Database.h"
#include "GeneratedModels.h"
#include <unistd.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void home(HTTPRequest *request) {
    render_html(request, "home.html", NULL, 0);
}

void create_user_view(HTTPRequest *request) {

    // ---- Create test user ----
    User u = {
        .DNI = "1",
        .name = "John",
        .age = 30,
        .email = "john@example.com",
        .group_id = 1
    };

    const char *result = "User created successfully ✅";

    if (!User_create(db, &u)) {
        result = "Failed to create user ❌";
    }

    // ---- Fetch user DNI from route parameters ----
    const char *dni_param = NULL;
    for (size_t i = 0; i < request->param_count; i++) {
        if (strcmp(request->params[i].key, "DNI") == 0) {
            dni_param = request->params[i].value;
            break;
        }
    }

    if (!dni_param) {
        result = "No DNI parameter provided in request ❌";
        dni_param = u.DNI; // fallback to created user
    }

    // ---- Fetch user from DB using DNI ----
    User db_user = {0};
    bool loaded = User_read(db, (char *)dni_param, &db_user); // cast is safe if User_read doesn't modify the string

    if (!loaded) {
        result = "User created but failed to read from DB ❌";
        db_user = u; // fallback
    }

    // ---- Template parameters ----
    TemplateParam params[] = {
        { "result",   result,             convert_string },
        { "name",     db_user.name,       convert_string },
        { "DNI",      db_user.DNI,        convert_string },
        { "age",      &db_user.age,       convert_int    },
        { "email",    db_user.email,      convert_string },
        { "group_id", &db_user.group_id,  convert_int    },
    };

    render_html(request, "create_user.html", params, 6);

    // ---- Cleanup strdup'ed DB strings ----
    if (loaded) {
        free(db_user.DNI);
        free(db_user.name);
        free(db_user.email);
    }
}
