#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 5001
#define BUF_SIZE 8192
#define TOTAL_SEND (200 * 1024 * 1024)

int main()
{
    int sock;

    struct sockaddr_in server;

    char buffer[BUF_SIZE];

    sock = socket(AF_INET, SOCK_STREAM, 0);

    memset(&server, 0, sizeof(server));

    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);

    inet_pton(AF_INET, "192.168.23.230", &server.sin_addr);

    connect(sock, (struct sockaddr*)&server, sizeof(server));

    printf("Connected\n");

    memset(buffer, 'A', BUF_SIZE);

    long long sent = 0;

    while (sent < TOTAL_SEND)
    {
        int n = write(sock, buffer, BUF_SIZE);

        if (n <= 0)
            break;

        sent += n;
    }

    printf("Sent %lld bytes\n", sent);

    close(sock);

    return 0;
}