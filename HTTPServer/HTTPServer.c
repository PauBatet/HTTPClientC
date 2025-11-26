#include "HTTPServer.h"
#include<sys/types.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<unistd.h>
#include<stdio.h>
#include<string.h>
#include<arpa/inet.h>
#include<stdlib.h>

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

	int client_socket;
	struct sockaddr_in client_address;
	socklen_t addrlen = sizeof(client_address);
	client_socket = accept(server->server_fd, (struct sockaddr *)&client_address, &addrlen);

	if (client_socket < 0) {
		perror("Failed to accept connection");
		return request;
	}

	char buffer[4096] = {0};
	int bytes_read = read(client_socket, buffer, sizeof(buffer) -1);
	if (bytes_read <= 0) {
		perror("Failed to read request");
		close(client_socket);
		return request;
	}

	sscanf(buffer, "%s %s %s", request.method, request.path, request.version);
	char *headers_start = strstr(buffer, "\r\n");
	char *body_start = strstr(buffer, "\r\n\r\n");
	
	if (headers_start) {
		size_t headers_len = body_start?(size_t)(body_start - headers_start):strlen(headers_start);
		strncpy(request.headers, headers_start+2, headers_len-2);
	}

	if (body_start) {
		strncpy(request.body, body_start+4, sizeof(request.body)-1);
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
			"Content-Length %d\r\n"
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
