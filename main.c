#include<sys/types.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<unistd.h>
#include<stdio.h>
#include<string.h>
#include<arpa/inet.h>

int main() {
	//Create socket
	printf("Creating socket");
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
		
	//Bind socket to ip and port
	printf("Binding the socket");
	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr("127.0.0.1");
	address.sin_port = htons(8080);
	
	//Listen
	printf("Now listening");
	listen(server_fd, 3);
	
	while (1) {
	//HandShake (Parallelize here)
		int addrlen = sizeof(address);
		int new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
	
		if (new_socket < 0) {
			perror("Failed to accept connection");
			continue;
		}

		//Read data
		char buffer[1024] = {0};
		read(new_socket, buffer, 1024);
		printf("Client message: %s\n", buffer);

		//Send data
		char *response = "Hello World!";
		write (new_socket, response, strlen(response));

		//Close
		close(new_socket);
	}
	close(server_fd);
	
	return 0;
}
