#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <stdlib.h>

#define DEFAULT_PORT 5001
#define BUF_SIZE 65536

static double now_sec()
{
    struct timeval tv;

    gettimeofday(&tv, NULL);

    return tv.tv_sec + tv.tv_usec / 1e6;
}

void usage(const char *prog)
{
    printf("\nUsage:\n");

    printf("  %s [-p <port>]\n",
           prog);

    printf("\nOptions:\n");

    printf("  -p <port>        TCP port (default: %d)\n",
           DEFAULT_PORT);

    printf("  -h, --help       Show this help message\n");

    printf("\n");
}

int main(int argc, char *argv[])
{
    int server_fd;
    int client_fd;

    int port = DEFAULT_PORT;

    // Parse arguments
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-p") == 0 &&
            i + 1 < argc)
        {
            port = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "-h") == 0 ||
                 strcmp(argv[i], "--help") == 0)
        {
            usage(argv[0]);
            return 0;
        }
        else
        {
            printf("Unknown option: %s\n",
                   argv[i]);

            usage(argv[0]);

            return -1;
        }
    }

    struct sockaddr_in addr;

    char buffer[BUF_SIZE];

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0)
    {
        perror("socket");
        return -1;
    }

    int opt = 1;

    setsockopt(server_fd,
               SOL_SOCKET,
               SO_REUSEADDR,
               &opt,
               sizeof(opt));

    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    // Bind socket
    if (bind(server_fd,
             (struct sockaddr*)&addr,
             sizeof(addr)) < 0)
    {
        perror("bind");

        close(server_fd);

        return -1;
    }

    // Start listening
    listen(server_fd, 1);

    printf("Server listening on %d...\n",
           port);

    // Accept client
    client_fd = accept(server_fd, NULL, NULL);

    if (client_fd < 0)
    {
        perror("accept");

        close(server_fd);

        return -1;
    }

    printf("Client connected\n\n");

    printf("%-18s %-18s\n",
           "Interval(sec)",
           "Bandwidth(MB/s)");

    printf("------------------------------------------------\n");

    long long total_bytes = 0;
    long long interval_bytes = 0;

    double start = now_sec();
    double last = start;

    while (1)
    {
        ssize_t n = read(client_fd,
                         buffer,
                         BUF_SIZE);

        if (n <= 0)
            break;

        total_bytes += n;
        interval_bytes += n;

        double now = now_sec();

        if (now - last >= 1.0)
        {
            double bandwidth =
                (interval_bytes / 1024.0 / 1024.0) /
                (now - last);

            printf("[%6.2f - %6.2f]   %10.2f\n",
                   last - start,
                   now - start,
                   bandwidth);

            interval_bytes = 0;
            last = now;
        }
    }

    double end = now_sec();

    // Last interval
    if (interval_bytes > 0)
    {
        double bandwidth =
            (interval_bytes / 1024.0 / 1024.0) /
            (end - last);

        printf("[%6.2f - %6.2f]   %10.2f (last)\n",
               last - start,
               end - start,
               bandwidth);
    }

    printf("\n===== FINAL =====\n");

    printf("Port        : %d\n", port);

    printf("Total bytes : %lld\n", total_bytes);

    printf("Time        : %.3f sec\n",
           end - start);

    printf("Avg MB/s    : %.2f\n",
           (total_bytes / 1024.0 / 1024.0) /
           (end - start));

    close(client_fd);
    close(server_fd);

    return 0;
}