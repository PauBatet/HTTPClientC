#include<sys/types.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<unistd.h>
#include<stdio.h>
#include<string.h>
#include<arpa/inet.h>
#include<stdlib.h>

int main() {
	//Create socket
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0) {
		perror("Socket creation failed");
		exit(EXIT_FAILURE);
	}
	
	//Allow address reuse
	int opt = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		perror("Setsockopt failed");
		close(server_fd);
		exit(EXIT_FAILURE);
	}
	//Bind socket to ip and port
	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr("127.0.0.1");
	address.sin_port = htons(8080);
	
	if(bind(server_fd, (struct sockaddr *)&address,sizeof(address)) < 0) {
		perror("Bind failed");
		close(server_fd);
		exit(EXIT_FAILURE);
	}

	//Listen
	printf("Now listening\n");
	fflush(stdout);
	if(listen(server_fd, 3) < 0) {
		perror("Listen failed");
		close(server_fd);
		exit(EXIT_FAILURE);

	}
	
	while (1) {
		//HandShake (Parallelize here)
		int addrlen = sizeof(address);
		int new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
	
		if (new_socket < 0) {
			perror("Failed to accept connection");
			continue;
		}

		//Read data
		char buffer[1024] = {0};
		ssize_t bytes_read = read(new_socket, buffer, sizeof(buffer)-1);
		if(bytes_read < 0) {
			perror("Read failed");
		}
		else {
			printf("Client message: %s\n", buffer);
		}

		//Send data
		char *response = "Hello World!";
		write(new_socket, response, strlen(response));

		//Close
		close(new_socket);
	}
	close(server_fd);
	return 0;
}
