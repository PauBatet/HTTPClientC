#include "HTTPFramework.h"
#include "Database.h"
#include "sqlite3.h"
#include "GeneratedModels.h"
#include <unistd.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void home(HTTPRequest *request) {
    render_html(request, "home.html", NULL, 0);
}

void about(HTTPRequest *request) {
    /* Increment counter */
    db_exec(db, "UPDATE visits SET count = count + 1 WHERE id = 1;");
    db_exec(db, "COMMIT;");
    /* Query value */
    void *stmt;
    int visits = 0;

    if (db_prepare(db,
            "SELECT count FROM visits WHERE id = 1;",
            &stmt))
    {
        if (db_step(stmt)) {
            visits = sqlite3_column_int(stmt, 0);
        }
        db_finalize(stmt);
    }

    /* Template param */
    TemplateParam params[] = {
        {
            .key = "visits",
            .value = &visits,
            .converter = convert_int
        }
    };

    render_html(
        request,
        "about.html",
        params,
        1
    );
}


void create_user_view(HTTPRequest *request) {

    User u = {
        .name = "John",
        .age = 30,
        .email = "john@example.com",
        .group_id = 1
    };
    
    const char *result;
    if (User_create(db, &u)){
        result = "<p>User created successfully</p>";
    }
    else {
        result = "<p>Failed to create user</p>";
    }

    /* Template param */
    TemplateParam params[] = {
        {
            .key = "result",
            .value = result,
            .converter = convert_string
        }
    };

    render_html(
        request,
        "create_user.html",
        params,
        1
    );
}

