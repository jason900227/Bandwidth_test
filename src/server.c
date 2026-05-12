#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 5001
#define BUF_SIZE 1024

int main()
{
    int server_fd;
    int client_fd;

    struct sockaddr_in addr;

    char buffer[BUF_SIZE];

    // Create TCP socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    // Clear address structure
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    // Bind socket to port
    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));

    // Start listening
    listen(server_fd, 1);

    printf("Server listening on port %d...\n", PORT);

    // Accept client connection
    client_fd = accept(server_fd, NULL, NULL);

    printf("Client connected\n");

    // Receive message
    int n = read(client_fd, buffer, BUF_SIZE - 1);

    if (n > 0)
    {
        buffer[n] = '\0';
        printf("Received: %s\n", buffer);
    }

    close(client_fd);
    close(server_fd);

    return 0;
}