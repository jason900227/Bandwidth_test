#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>

#define PORT 5001
#define BUF_SIZE 8192

static double now_sec()
{
    struct timeval tv;

    gettimeofday(&tv, NULL);

    return tv.tv_sec + tv.tv_usec / 1e6;
}

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
    long long interval_bytes = 0;

    ssize_t n;

    double start = now_sec();
    double last = start;

    while ((n = read(client_fd, buffer, BUF_SIZE)) > 0)
    {
        total_bytes += n;
        interval_bytes += n;

        double now = now_sec();

        if (now - last >= 1.0)
        {
            double bandwidth =
                (interval_bytes / 1024.0 / 1024.0) /
                (now - last);

            printf("[ %.2f - %.2f sec ] %.2f MB/s\n",
                   last - start,
                   now - start,
                   bandwidth);

            interval_bytes = 0;
            last = now;
        }
    }

    double end = now_sec();

    // Handle last interval
    if (interval_bytes > 0)
    {
        double bandwidth =
            (interval_bytes / 1024.0 / 1024.0) /
            (end - last);

        printf("[ %.2f - %.2f sec ] %.2f MB/s (last)\n",
               last - start,
               end - start,
               bandwidth);
    }

    double avg =
        (total_bytes / 1024.0 / 1024.0) /
        (end - start);

    printf("\n===== FINAL =====\n");
    printf("Total: %lld bytes\n", total_bytes);
    printf("Time : %.2f sec\n", end - start);
    printf("Avg  : %.2f MB/s\n", avg);

    close(client_fd);
    close(server_fd);

    return 0;
}