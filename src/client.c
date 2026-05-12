#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 5001

int main()
{
    int sock;

    struct sockaddr_in server;

    char message[] = "Hello Server";

    // Create TCP socket
    sock = socket(AF_INET, SOCK_STREAM, 0);

    // Clear server structure
    memset(&server, 0, sizeof(server));

    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);

    // Convert IP address
    inet_pton(AF_INET, "192.168.23.230", &server.sin_addr);

    // Connect to server
    connect(sock, (struct sockaddr*)&server, sizeof(server));

    // Send message
    write(sock, message, strlen(message));

    printf("Message sent\n");

    close(sock);

    return 0;
}