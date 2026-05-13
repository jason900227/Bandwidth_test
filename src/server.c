#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdint.h>

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

    printf("  %s -p <port>\n",
           prog);

    printf("\nOptions:\n");

    printf("  -p <port>        TCP port (default: %d)\n",
           DEFAULT_PORT);

    printf("  -h, --help       Show this help message\n");

    printf("\n");
}

static void print_interval(double last, double now, double start, long long bytes)
{
    double bandwidth = (bytes / 1024.0 / 1024.0) / (now - last);

    printf("[%6.2f - %6.2f]   %10.2f\n",
           last - start,
           now - start,
           bandwidth);
}

static long long do_recv(int fd, char *buf, unsigned dur)
{
    long long total_bytes    = 0;
    long long interval_bytes = 0;
    unsigned  printed        = 0;

    double start = now_sec();
    double last  = start;

    while (1)
    {
        ssize_t n = read(fd, buf, BUF_SIZE);

        if (n <= 0)
            break;

        total_bytes    += n;
        interval_bytes += n;

        double now = now_sec();

        if (now - last >= 1.0 && printed < dur)
        {
            print_interval(last, now, start, interval_bytes);

            printed++;
            interval_bytes = 0;
            last = now;
        }
    }

    if (interval_bytes > 0 && printed < dur)
        print_interval(last, now_sec(), start, interval_bytes);

    return total_bytes;
}

static long long do_send(int fd, char *buf, unsigned dur)
{
    long long total_bytes    = 0;
    long long interval_bytes = 0;
    unsigned  printed        = 0;

    double start = now_sec();
    double last  = start;

    while (now_sec() - start < dur)
    {
        ssize_t n = write(fd, buf, BUF_SIZE);

        if (n <= 0)
        {
            if (printed < dur)
                print_interval(last, now_sec(), start, interval_bytes);

            return total_bytes;
        }

        total_bytes    += n;
        interval_bytes += n;

        double now = now_sec();

        if (now - last >= 1.0 && printed < dur)
        {
            print_interval(last, now, start, interval_bytes);

            printed++;
            interval_bytes = 0;
            last = now;
        }
    }

    if (printed < dur)
        print_interval(last, now_sec(), start, interval_bytes);

    return total_bytes;
}

int main(int argc, char *argv[])
{
    int server_fd;
    int client_fd;

    int port = DEFAULT_PORT;

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

    if (bind(server_fd,
             (struct sockaddr*)&addr,
             sizeof(addr)) < 0)
    {
        perror("bind");

        close(server_fd);

        return -1;
    }

    listen(server_fd, 1);

    printf("Server listening on %d...\n",
           port);

    client_fd = accept(server_fd, NULL, NULL);

    if (client_fd < 0)
    {
        perror("accept");

        close(server_fd);

        return -1;
    }

    printf("Client connected\n");

    /*
     * Receive handshake
     */
    control_t ctrl;

    memset(&ctrl, 0, sizeof(ctrl));

    read(client_fd, &ctrl, sizeof(ctrl));

    if (ctrl.magic != MAGIC)
    {
        printf("Bad protocol\n");

        close(client_fd);
        close(server_fd);

        return -1;
    }

    printf("Mode        : ");

    if (ctrl.mode == MODE_UP)
        printf("UPSTREAM\n");
    else if (ctrl.mode == MODE_DOWN)
        printf("DOWNSTREAM\n");
    else if (ctrl.mode == MODE_BOTH)
        printf("BOTH\n");

    printf("Duration    : %u sec\n\n", ctrl.duration);

    printf("%-18s %-18s\n",
           "Interval(sec)",
           "Bandwidth(MB/s)");

    printf("------------------------------------------------\n");

    memset(buffer, 'A', BUF_SIZE);

    /*
     * UPSTREAM
     */
    if (ctrl.mode == MODE_UP)
    {
        do_recv(client_fd, buffer, ctrl.duration);
    }

    /*
     * DOWNSTREAM
     */
    else if (ctrl.mode == MODE_DOWN)
    {
        do_send(client_fd, buffer, ctrl.duration);
    }

    /*
     * BOTH
     */
    else if (ctrl.mode == MODE_BOTH)
    {
        printf("UPSTREAM phase\n");

        do_recv(client_fd, buffer, ctrl.duration);

        printf("\nDOWNSTREAM phase\n");

        do_send(client_fd, buffer, ctrl.duration);
    }

    close(client_fd);
    close(server_fd);

    return 0;
}