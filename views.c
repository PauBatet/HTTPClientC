#include"HTTPFramework.h"

void home (HTTPRequest *request) {
	render_html(request, "home.html");
}

void about (HTTPRequest *request) {
	const char *body = "<h1>Hello, this is my About Page</h1>";
	HTTPServer_send_response(request, body,"",0,"");
}
