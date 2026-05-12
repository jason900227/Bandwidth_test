#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <time.h>

#define DEFAULT_PORT 5001
#define BUF_SIZE 65536

void usage(const char *prog)
{
    printf("\nUsage:\n");

    printf("  %s -c <server_ip> -p <port> -t <seconds>\n",
           prog);

    printf("\nOptions:\n");

    printf("  -c <server_ip>   Server IP address\n");

    printf("  -p <port>        TCP port (default: %d)\n",
           DEFAULT_PORT);

    printf("  -t <seconds>     Test duration in seconds (default: 5)\n");

    printf("  -h, --help       Show this help message\n");

    printf("\n");
}

int main(int argc, char *argv[])
{
    int sock;

    struct sockaddr_in server;

    char buffer[BUF_SIZE];

    char *ip = NULL;

    int port = DEFAULT_PORT;

    int duration = 5;

    // Parse arguments (order-independent)
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-c") == 0 && i + 1 < argc)
        {
            ip = argv[++i];
        }
        else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc)
        {
            port = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc)
        {
            duration = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "-h") == 0 ||
                 strcmp(argv[i], "--help") == 0)
        {
            usage(argv[0]);
            return 0;
        }
        else
        {
            printf("Unknown option: %s\n", argv[i]);

            usage(argv[0]);

            return -1;
        }
    }

    if (!ip || duration <= 0)
    {
        usage(argv[0]);
        return -1;
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0)
    {
        perror("socket");
        return -1;
    }

    memset(&server, 0, sizeof(server));

    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &server.sin_addr) <= 0)
    {
        printf("Invalid IP address\n");
        close(sock);
        return -1;
    }

    printf("Connecting to %s:%d...\n", ip, port);

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0)
    {
        perror("connect");
        close(sock);
        return -1;
    }

    printf("Connected\n");

    memset(buffer, 'A', BUF_SIZE);

    time_t start = time(NULL);

    long long sent = 0;

    while (1)
    {
        if (time(NULL) - start >= duration)
            break;

        int n = write(sock, buffer, BUF_SIZE);

        if (n <= 0)
        {
            perror("write");
            break;
        }

        sent += n;
    }

    printf("\n===== CLIENT RESULT =====\n");

    printf("Server IP   : %s\n", ip);

    printf("Port        : %d\n", port);

    printf("Duration    : %d sec\n", duration);

    printf("Bytes Sent  : %lld\n", sent);

    printf("MB Sent     : %.2f MB\n",
           sent / 1024.0 / 1024.0);

    close(sock);

    return 0;
}