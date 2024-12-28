#include"HTTPFramework.h"
#include<unistd.h> 
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

void home(HTTPRequest *request) {
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

    render_html(request, "home.html", params, 7);
}

void about (HTTPRequest *request) {
	const char *body = "<h1>Hello, this is my About Page</h1>";
	HTTPServer_send_response(request, body,"",0,"");
}

void abt (HTTPRequest *request) {
	render_html(request, "abt.html", NULL, 0);
}

void Delay30(HTTPRequest *request) {
    // Log that the delay is starting
    printf("Delay30: Simulating a long operation for 30 seconds.\n");

    // Sleep for 30 seconds
    sleep(30);

    // Respond after the delay
    const char *body = "<h1>Delay complete. Thank you for waiting!</h1>";
    HTTPServer_send_response(request, body, "", 0, "");
}
