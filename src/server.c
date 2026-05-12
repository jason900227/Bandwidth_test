#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 5001
#define BUF_SIZE 8192

int main()
{
    int server_fd;
    int client_fd;

    struct sockaddr_in addr;

    char buffer[BUF_SIZE];

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));

    listen(server_fd, 1);

    printf("Server listening...\n");

    client_fd = accept(server_fd, NULL, NULL);

    printf("Client connected\n");

    long long total_bytes = 0;

    ssize_t n;

    time_t start = time(NULL);

    while ((n = read(client_fd, buffer, BUF_SIZE)) > 0)
    {
        total_bytes += n;
    }

    time_t end = time(NULL);

    double duration = end - start;

    if (duration <= 0)
        duration = 1;

    printf("\n===== RESULT =====\n");
    printf("Received: %lld bytes\n", total_bytes);
    printf("Time: %.2f sec\n", duration);
    printf("Bandwidth: %.2f MB/s\n",
           (total_bytes / 1024.0 / 1024.0) / duration);

    close(client_fd);
    close(server_fd);

    return 0;
}