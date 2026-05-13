#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>

#define DEFAULT_PORT 5001
#define BUF_SIZE 65536

#define MAGIC 0x12345678

#define MODE_UP   1
#define MODE_DOWN 2
#define MODE_BOTH 3

typedef struct
{
    uint32_t magic;
    uint32_t mode;
    uint32_t duration;
} control_t;

static double now_sec()
{
    struct timeval tv;

    gettimeofday(&tv, NULL);

    return tv.tv_sec + tv.tv_usec / 1e6;
}

void usage(const char *prog)
{
    printf("\nUsage:\n");

    printf("  %s -c <server_ip> -p <port> -t <seconds> -m <mode>\n",
           prog);

    printf("\nOptions:\n");

    printf("  -c <server_ip>   Server IP address\n");

    printf("  -p <port>        TCP port (default: %d)\n",
           DEFAULT_PORT);

    printf("  -t <seconds>     Test duration in seconds (default: 5)\n");

    printf("  -m <mode>        up | down | both (default: up)\n");

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

    int mode = MODE_UP;

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
        else if (strcmp(argv[i], "-m") == 0 && i + 1 < argc)
        {
            i++;

            if (strcmp(argv[i], "up") == 0)
            {
                mode = MODE_UP;
            }
            else if (strcmp(argv[i], "down") == 0)
            {
                mode = MODE_DOWN;
            }
            else if (strcmp(argv[i], "both") == 0)
            {
                mode = MODE_BOTH;
            }
            else
            {
                printf("Invalid mode: %s\n", argv[i]);
                return -1;
            }
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

    /*
     * Send handshake
     */
    control_t ctrl;

    ctrl.magic    = MAGIC;
    ctrl.mode     = mode;
    ctrl.duration = duration;

    write(sock, &ctrl, sizeof(ctrl));

    memset(buffer, 'A', BUF_SIZE);

    long long up_bytes   = 0;
    long long down_bytes = 0;
    double    start;
    ssize_t   n;

    /*
     * UPSTREAM
     */
    if (mode == MODE_UP || mode == MODE_BOTH)
    {
        printf(mode == MODE_UP ? "Mode: UPSTREAM\n" : "Mode: BOTH\nUPSTREAM phase\n");

        start = now_sec();

        while (now_sec() - start < duration)
        {
            n = write(sock, buffer, BUF_SIZE);

            if (n <= 0)
            {
                perror("write");
                break;
            }

            up_bytes += n;
        }

        if (mode == MODE_BOTH)
            shutdown(sock, SHUT_WR);
    }

    /*
     * DOWNSTREAM
     */
    if (mode == MODE_DOWN || mode == MODE_BOTH)
    {
        printf(mode == MODE_DOWN ? "Mode: DOWNSTREAM\n" : "DOWNSTREAM phase\n");

        start = now_sec();

        while (now_sec() - start < duration)
        {
            n = read(sock, buffer, BUF_SIZE);

            if (n <= 0)
            {
                perror("read");
                break;
            }

            down_bytes += n;
        }

        sleep(1);
    }

    /*
     * RESULT
     */
    printf("\nServer IP   : %s\n", ip);
    printf("Port        : %d\n", port);
    printf("Duration    : %d sec\n", duration);

    if (mode == MODE_UP)
    {
        printf("\n===== UPSTREAM RESULT =====\n");
        printf("Bytes Sent  : %lld\n", up_bytes);
        printf("MB Sent     : %.2f MB\n", up_bytes / 1024.0 / 1024.0);
        printf("Avg MB/s    : %.2f\n", (up_bytes / 1024.0 / 1024.0) / duration);
    }
    else if (mode == MODE_DOWN)
    {
        printf("\n===== DOWNSTREAM RESULT =====\n");
        printf("Bytes Recv  : %lld\n", down_bytes);
        printf("MB Recv     : %.2f MB\n", down_bytes / 1024.0 / 1024.0);
        printf("Avg MB/s    : %.2f\n", (down_bytes / 1024.0 / 1024.0) / duration);
    }
    else if (mode == MODE_BOTH)
    {
        printf("\n===== UPSTREAM RESULT =====\n");
        printf("Bytes Sent  : %lld\n", up_bytes);
        printf("MB Sent     : %.2f MB\n", up_bytes / 1024.0 / 1024.0);
        printf("Avg MB/s    : %.2f\n", (up_bytes / 1024.0 / 1024.0) / duration);

        printf("\n===== DOWNSTREAM RESULT =====\n");
        printf("Bytes Recv  : %lld\n", down_bytes);
        printf("MB Recv     : %.2f MB\n", down_bytes / 1024.0 / 1024.0);
        printf("Avg MB/s    : %.2f\n", (down_bytes / 1024.0 / 1024.0) / duration);

        printf("\n===== TOTAL =====\n");
        long long total = up_bytes + down_bytes;
        printf("Bytes Total : %lld\n", total);
        printf("MB Total    : %.2f MB\n", total / 1024.0 / 1024.0);
        printf("Avg MB/s    : %.2f\n", (total / 1024.0 / 1024.0) / (duration * 2));
    }

    close(sock);

    return 0;
}