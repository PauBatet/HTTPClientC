#include"HTTPFramework.h"
#include<stdio.h>
#include<string.h>

void handle_request(HTTPRequest *request) {
	for(int i = 0; routes[i].path != NULL; i++) {
		if(strcmp(request->path, routes[i].path) == 0) {
			routes[i].handler(request);
			return;
		}
	}
	HTTPServer_send_response(request, "", "", 404, "<h1>404 Not Found</h1>");
}

int main() {
	HTTPServer *server = HTTPServer_create(8080);
	if (!server) {
		printf("Failed to create server\n");
		return 1;
	}

	while (1) {
		HTTPRequest request = HTTPServer_listen(server);

		if (strlen(request.method)==0){
			printf("Invalid Request\n");
			continue;
		}

		printf("%s %s", request.method, request.path);
		
		handle_request(&request);
	}

	HTTPServer_destroy(server);
	return 0;
}
