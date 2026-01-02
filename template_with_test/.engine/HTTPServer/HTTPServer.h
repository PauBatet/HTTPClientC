#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <stdbool.h>
#include<netinet/in.h>

typedef struct {
    char *key;
    char *value;
} HTTPParam;

typedef struct {
    char *key;
    char *value;
} HTTPHeader;

typedef struct {
    char method[8];
    char version[16];

    char *path;
    char *headers;
    char *body;

    size_t headers_len;
    size_t body_len;

    HTTPParam *params;
    size_t param_count;
    size_t param_capacity;

    HTTPHeader *header_list;
    size_t header_count;
    size_t header_capacity;

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

void HTTPRequest_free(HTTPRequest *req);

bool HTTPRequest_add_param(HTTPRequest *req, const char *key, const char *value);

const char *HTTPRequest_get_param(HTTPRequest *req, const char *key);

bool HTTPRequest_add_header(HTTPRequest *req, const char *key, const char *value);

const char *HTTPRequest_get_header(HTTPRequest *req, const char *key);

#endif
