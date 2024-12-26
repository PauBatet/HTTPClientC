#include"HTMLTemplating.h"
#include<stdio.h>
#include<stdlib.h>

void render_html(HTTPRequest *request, const char *file_path) {
	FILE *file = fopen(file_path, "r");
	if (!file) {
		const char *body = "<h1>404 Not Found</h1>";
		HTTPServer_send_response(request, body, "", 404, "");
		return;
	}

	fseek(file, 0, SEEK_END);
	long file_size = ftell(file);
	fseek(file, 0, SEEK_SET);

	char *file_content = (char *)malloc(file_size+1);
	if (!file_content) {
		const char *body = "<h1>500 Internal Server Error</h1>";
		HTTPServer_send_response(request, body, "", 500, "");
		fclose(file);
		return;
	}

	fread(file_content, 1, file_size, file);
	file_content[file_size] = '\0';

	fclose(file);

	HTTPServer_send_response(request, file_content, "", 0, "");

	free(file_content);
}
