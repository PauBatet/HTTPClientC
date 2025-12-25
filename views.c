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

void example(HTTPRequest *request) {
    // Create a struct to hold profile information
        char name[100] = "";
		snprintf(name, sizeof(name), (1)?"Pau Batet Castells":"Someone else");
        char title[200] = "Web Developer | Designer | Creator";
        char about[1000] = "So yeah, this is me, you might be wondering how I got here. Well, it's a long story.";
        char image_url[200] = "https://avatars.githubusercontent.com/u/125908500?v=4";

    // Create arrays for projects and jobs
    struct Project {
        char title[100];
        char description[500];
    } projects[] = {
        {"Project 1", "Lorem ipsum dolor sit amet, consectetur adipiscing elit."},
        {"Project 2", "Lorem ipsum dolor sit amet, consectetur adipiscing elit."},
        {"Project 3", "Lorem ipsum dolor sit amet, consectetur adipiscing elit."}
    };

    struct Job {
        char title[100];
        char description[500];
    } jobs[] = {
        {"Job 1", "Lorem ipsum dolor sit amet, consectetur adipiscing elit."},
        {"Job 2", "Lorem ipsum dolor sit amet, consectetur adipiscing elit."},
        {"Job 3", "Lorem ipsum dolor sit amet, consectetur adipiscing elit."}
    };

    char* convert_projects(const void* value) {
        const struct Project* projects = (const struct Project*)value;
        // Allocate a large buffer for the HTML
        char* buffer = malloc(4096);  // Adjust size as needed
        buffer[0] = '\0';
        
        for (int i = 0; i < 3; i++) {  // Assuming 3 projects
            char project_html[1024];
            snprintf(project_html, sizeof(project_html),
                "<div class=\"project\">"
                    "<div>"
                        "<h3>%s</h3>"
                        "<p>%s</p>"
                    "</div>"
                    "<a href=\"#\" class=\"button\">View</a>"
                "</div>",
                projects[i].title, projects[i].description
            );
            strcat(buffer, project_html);
        }
        
        return buffer;
    }

    char *jobsTemplate = malloc(4096);
    jobsTemplate[0] = '\0';

    for (int i = 0; i < 3; i++) {
        TemplateParam params[] = {
            {"title", jobs[i].title, NULL},
            {"description", jobs[i].description, NULL}
        };
        strcat(jobsTemplate, process_html("label_content.html", params, 2));
    }

    // Create the template parameters
    TemplateParam params[] = {
        {"name", &name, NULL},
        {"title", &title, NULL},
        {"about", &about, NULL},
        {"image_url", &image_url, NULL},
        {"year", "2024", NULL},
        {"projects", projects, convert_projects},
        {"jobs", jobsTemplate, NULL}
    };

    render_html(request, "example.html", params, 7);
    free(jobsTemplate);
}
