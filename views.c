#include"HTTPFramework.h"
#include<unistd.h> 
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

void home(HTTPRequest *request) {
    render_html(request, "home.html", NULL, 0);
}

void about(HTTPRequest *request) {
    render_html(request, "subfolder/about.html", NULL, 0);
}
