#include"HTTPServer.h"
#include<stdio.h>
#include<string.h>

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

		if (strcmp(request.path, "/")==0){
			const char *body = "<h1>Welcome to my HTTP serrver!</h1>";
			HTTPServer_send_response(&request, body,"",0,"");
		}
		else {
			const char *body = "";
			HTTPServer_send_response(&request, body, "text/html", 404, "");
		}
	}

	HTTPServer_destroy(server);
	return 0;
}
