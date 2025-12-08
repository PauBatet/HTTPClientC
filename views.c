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

    // Create test user
    User u = {
        .name = "John",
        .age = 30,
        .email = "john@example.com",
        .group_id = 1
    };

    const char *result = "User created successfully ✅";

    if (!User_create(db, &u)) {
        result = "Failed to create user ❌";
    }

    // ---- Fetch freshly created user from DB ----
    User db_user = {0};

    bool loaded = User_read(db, u.DNI, &db_user);

    if (!loaded) {
        result = "User created but failed to read from DB ❌";
        // fallback to original struct so page still renders
        db_user = u;
    }

    // ---- Template parameters from DB result ----
    TemplateParam params[] = {
        { "result",   result,             convert_string },
        { "name",     db_user.name,       convert_string },
        { "DNI",      db_user.DNI,        convert_string },
        { "age",      &db_user.age,       convert_int    },
        { "email",    db_user.email,      convert_string },
        { "group_id", &db_user.group_id,  convert_int    },
    };

    render_html(
        request,
        "create_user.html",
        params,
        6
    );

    // ---- Cleanup strdup'ed DB strings ----
    if (loaded) {
        free(db_user.DNI);
        free(db_user.name);
        free(db_user.email);
    }
}
