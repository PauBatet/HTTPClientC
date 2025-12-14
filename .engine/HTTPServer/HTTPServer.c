#include "HTTPServer.h"
#include<sys/types.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<unistd.h>
#include<stdio.h>
#include<string.h>
#include<arpa/inet.h>
#include<stdlib.h>

bool HTTPRequest_add_header(HTTPRequest *req, const char *key, const char *value) {
    if (!req || !key || !value) return false;

    if (req->header_count == req->header_capacity) {
        req->header_capacity = req->header_capacity ? req->header_capacity * 2 : 8;
        req->header_list = realloc(
            req->header_list,
            req->header_capacity * sizeof(HTTPHeader)
        );

        if (!req->header_list) return false;
    }

    req->header_list[req->header_count].key   = strdup(key);
    req->header_list[req->header_count].value = strdup(value);
    req->header_count++;

    return true;
}

const char *HTTPRequest_get_header(HTTPRequest *req, const char *key) {
    if (!req || !key) return NULL;

    for (size_t i = 0; i < req->header_count; i++) {
        if (strcasecmp(req->header_list[i].key, key) == 0) {
            return req->header_list[i].value;
        }
    }

    return NULL;
}

static void parse_headers(HTTPRequest *req) {
    if (!req->headers) return;

    char *copy = strdup(req->headers);
    char *line = strtok(copy, "\r\n");

    while (line) {
        char *colon = strchr(line, ':');

        if (colon) {
            *colon = 0;

            char *key = line;
            char *value = colon + 1;

            // Skip leading spaces in value
            while (*value == ' ') value++;

            HTTPRequest_add_header(req, key, value);
        }

        line = strtok(NULL, "\r\n");
    }

    free(copy);
}

bool HTTPRequest_add_param(HTTPRequest *req, const char *key, const char *value) {
    if (!req || !key || !value) return false;
    if (req->param_count == req->param_capacity) {
        req->param_capacity = req->param_capacity ? req->param_capacity * 2 : 8;
        req->params = realloc(req->params, req->param_capacity * sizeof(HTTPParam));
        if (!req->params) return false;
    }
    req->params[req->param_count].key = strdup(key);
    req->params[req->param_count].value = strdup(value);
    req->param_count++;
    return true;
}

const char *HTTPRequest_get_param(HTTPRequest *req, const char *key) {
    if (!req || !key) return NULL;

    for (size_t i = 0; i < req->param_count; i++) {
        if (strcmp(req->params[i].key, key) == 0) {
            return req->params[i].value;
        }
    }

    return NULL;
}

static void parse_query_params(HTTPRequest *req, char *query) {
    char *pair = strtok(query, "&");

    while (pair) {
        char *eq = strchr(pair, '=');

        if (!HTTPRequest_add_param(req, pair, eq ? eq + 1 : "")) {
            perror("Failed to add query param");
            return;
        }
        pair = strtok(NULL, "&");
    }
}

void HTTPRequest_free(HTTPRequest *req) {
    free(req->path);
    free(req->headers);
    free(req->body);

    for (size_t i = 0; i < req->param_count; i++) {
        free(req->params[i].key);
        free(req->params[i].value);
    }
    free(req->params);

     for (size_t i = 0; i < req->header_count; i++) {
        free(req->header_list[i].key);
        free(req->header_list[i].value);
    }
    free(req->header_list);
}

HTTPServer *HTTPServer_create(int port) {
	HTTPServer *server=malloc(sizeof(HTTPServer));
	if (!server) {
		perror("Failed to allocate server");
		return NULL;
	}

	server->port=port;

	server->server_fd=socket(AF_INET, SOCK_STREAM, 0);
	if(server->server_fd < 0) {
		perror("Socket creation failed");
		free(server);
		return NULL;
	}

	int opt = 1;
	setsockopt(server->server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	server->address.sin_family = AF_INET;
	server->address.sin_addr.s_addr = INADDR_ANY;
	server->address.sin_port = htons(port);

	if(bind(server->server_fd, (struct sockaddr *)&server->address, sizeof(server->address)) < 0) {
		perror("Bind failed");
		close(server->server_fd);
		free(server);
		return NULL;
	}

	if (listen(server->server_fd, 3) < 0) {
		perror("Listen failed");
		close(server->server_fd);
		free(server);
		return NULL;
	}

	printf("Server created on port http://localhost:%d\n", port);
	return server;
}

HTTPRequest HTTPServer_listen(HTTPServer *server) {
    HTTPRequest request = {0};
    request.params = NULL;
    request.param_count = 0;
    request.param_capacity = 0;

    struct sockaddr_in client_address;
    socklen_t addrlen = sizeof(client_address);
    int client_socket = accept(
        server->server_fd,
        (struct sockaddr *)&client_address,
        &addrlen
    );

    if (client_socket < 0) {
        perror("Failed to accept connection");
        return request;
    }

    char buffer[8192];
    int bytes = read(client_socket, buffer, sizeof(buffer) - 1);
    if (bytes <= 0) {
        close(client_socket);
        return request;
    }
    buffer[bytes] = '\0';

    char raw_path[1024];
    sscanf(buffer, "%s %1023s %s",
        request.method,
        raw_path,
        request.version
    );

    //SPLIT PATH & QUERY
    char *query = strchr(raw_path, '?');

    if (query) {
        *query = '\0';
        query++;
    }

    request.path = strdup(raw_path);

    if (query) {
        char *query_copy = strdup(query);
        parse_query_params(&request, query_copy);
        free(query_copy);
    }

    //HEADERS
    char *headers_start = strstr(buffer, "\r\n");
    char *body_start    = strstr(buffer, "\r\n\r\n");

    if (headers_start && body_start) {
        size_t len = body_start - headers_start - 2;
        request.headers = malloc(len + 1);
        memcpy(request.headers, headers_start + 2, len);
        request.headers[len] = 0;
        request.headers_len = len;
        request.header_list = NULL;
        request.header_count = 0;
        request.header_capacity = 0;

        parse_headers(&request);

    }

    //BODY
    if (body_start) {
        char *body = body_start + 4;

        request.body_len = strlen(body);
        request.body = strdup(body);
    }

    request.client_socket = client_socket;
    return request;
}

const char *get_default_status_message(int status_code) {
	switch(status_code) {
		case 200: return "OK";
		case 201: return "Created";
		case 400: return "Bad Request";
		case 404: return "Not Found";
		case 500: return "Internal Server Error";
		case 503: return "Service Unavailable";
		default: return "Unknown Status";
	}
}

void HTTPServer_send_response(HTTPRequest *request, const char *body, const char *content_type, int status_code, const char *status_message) {
	const char *final_status_message = (status_message && strlen(status_message) > 0)? status_message:get_default_status_message(status_code);
	const char *final_content_type = (content_type && strlen(content_type) > 0)? content_type: "text/html";	
	int final_status_code = (status_code > 0)?status_code:200;
	int content_length = strlen(body);
	char response_header[4096];
	snprintf(response_header, sizeof(response_header),
			"HTTP/1.1 %d %s\r\n"
			"Content-Type: %s\r\n"
			"Content-Length: %d\r\n"
			"\r\n",
			final_status_code, final_status_message, final_content_type, content_length);
	write(request->client_socket, response_header, strlen(response_header));
	write(request->client_socket, body, content_length);
	close(request->client_socket);
}

void HTTPServer_destroy(HTTPServer *server) {
	if (!server) return;

	close(server->server_fd);
	free(server);

	printf("Server shut down\n");
}
