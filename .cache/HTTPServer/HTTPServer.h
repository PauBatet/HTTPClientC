#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include<netinet/in.h>

typedef struct {
	char method[8];
	char path[256];
	char version[16];
	char headers[1024];
	char body[2048];
	int client_socket;
} HTTPRequest;

typedef struct {
	int server_fd;
	int port;
	struct sockaddr_in address;
}HTTPServer;

HTTPServer *HTTPServer_create(int port);
 
HTTPRequest HTTPServer_listen(HTTPServer *server);

void HTTPServer_send_response(HTTPRequest *request, const char *body, const char *content_type, int status_code, const char *status_message);

void HTTPServer_destroy(HTTPServer *server);

#endif
