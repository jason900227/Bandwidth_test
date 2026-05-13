#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>

#define DEFAULT_PORT  5001
#define BUF_SIZE      65536
#define HANDSHAKE_KEY 0x12345678

#define MODE_UP   1
#define MODE_DOWN 2
#define MODE_BOTH 3

typedef struct
{
    uint32_t key;
    uint32_t mode;
    uint32_t duration;
} control_t;

typedef struct
{
    long long bytes;
    double    elapsed;
} result_t;

static double now_sec()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1e6;
}

static void print_interval(double last, double now,
                            double start, long long bytes)
{
    double bandwidth = (bytes / 1024.0 / 1024.0) / (now - last);
    printf("[%6.2f - %6.2f]   %10.2f\n",
           last - start, now - start, bandwidth);
}

static void print_result(const char *label, const char *dir,
                         long long bytes, double elapsed)
{
    printf("\n===== %s RESULT =====\n", label);
    printf("Bytes %-5s : %lld\n",    dir, bytes);
    printf("MB    %-5s : %.2f MB\n", dir, bytes / 1024.0 / 1024.0);
    printf("Elapsed     : %.2f sec\n", elapsed);
    printf("Avg MB/s    : %.2f\n",  (bytes / 1024.0 / 1024.0) / elapsed);
}

void usage(const char *prog)
{
    printf("\nUsage:\n");
    printf("  %s -c <server_ip> [-p port] [-t sec] [-m up|down|both]\n", prog);
    printf("\nOptions:\n");
    printf("  -c <server_ip>   Server IP address\n");
    printf("  -p <port>        TCP port (default: %d)\n", DEFAULT_PORT);
    printf("  -t <seconds>     Test duration in seconds (default: 5)\n");
    printf("  -m <mode>        up | down | both (default: up)\n");
    printf("  -h, --help       Show this help message\n\n");
}

static result_t do_send(int fd, char *buf, int duration)
{
    long long total_bytes    = 0;
    long long interval_bytes = 0;
    unsigned  printed        = 0;
    double    start          = now_sec();
    double    last           = start;
    ssize_t   n;

    printf("%-18s %-18s\n", "Interval(sec)", "Bandwidth(MB/s)");
    printf("------------------------------------------------\n");

    while (now_sec() - start < duration)
    {
        n = write(fd, buf, BUF_SIZE);
        if (n <= 0) { perror("write"); break; }

        total_bytes    += n;
        interval_bytes += n;

        double now = now_sec();

        if (now - last >= 1.0 && printed < (unsigned)duration)
        {
            print_interval(last, now, start, interval_bytes);
            printed++;
            interval_bytes = 0;
            last = now;
        }
    }

    double elapsed = now_sec() - start;

    if (interval_bytes > 0 && printed < (unsigned)duration)
        print_interval(last, now_sec(), start, interval_bytes);

    return (result_t){ total_bytes, elapsed };
}

static result_t do_recv(int fd, char *buf, int duration)
{
    long long total_bytes    = 0;
    long long interval_bytes = 0;
    unsigned  printed        = 0;
    double    start          = now_sec();
    double    last           = start;
    ssize_t   n;

    printf("%-18s %-18s\n", "Interval(sec)", "Bandwidth(MB/s)");
    printf("------------------------------------------------\n");

    while ((n = read(fd, buf, BUF_SIZE)) > 0)
    {
        total_bytes    += n;
        interval_bytes += n;

        double now = now_sec();

        if (now - last >= 1.0 && printed < (unsigned)duration)
        {
            print_interval(last, now, start, interval_bytes);
            printed++;
            interval_bytes = 0;
            last = now;
        }
    }

    double elapsed = now_sec() - start;

    if (interval_bytes > 0 && printed < (unsigned)duration)
        print_interval(last, now_sec(), start, interval_bytes);

    return (result_t){ total_bytes, elapsed };
}

int main(int argc, char *argv[])
{
    int  sock;
    char *ip      = NULL;
    int  port     = DEFAULT_PORT;
    int  duration = 5;
    int  mode     = MODE_UP;

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
            if      (strcmp(argv[i], "up")   == 0) mode = MODE_UP;
            else if (strcmp(argv[i], "down") == 0) mode = MODE_DOWN;
            else if (strcmp(argv[i], "both") == 0) mode = MODE_BOTH;
            else { printf("Invalid mode: %s\n", argv[i]); return -1; }
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

    if (!ip || duration <= 0) { usage(argv[0]); return -1; }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); return -1; }

    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port   = htons(port);

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
    ctrl.key      = HANDSHAKE_KEY;
    ctrl.mode     = mode;
    ctrl.duration = duration;
    write(sock, &ctrl, sizeof(ctrl));

    char     buffer[BUF_SIZE];
    result_t up_res   = { 0, 0.0 };
    result_t down_res = { 0, 0.0 };

    memset(buffer, 'A', BUF_SIZE);

    /*
     * UPSTREAM
     */
    if (mode == MODE_UP || mode == MODE_BOTH)
    {
        printf(mode == MODE_UP ? "Mode: UPSTREAM\n" : "Mode: BOTH\nUPSTREAM phase\n");
        up_res = do_send(sock, buffer, duration);
        shutdown(sock, SHUT_WR);
    }

    /*
     * DOWNSTREAM
     */
    if (mode == MODE_DOWN || mode == MODE_BOTH)
    {
        printf(mode == MODE_DOWN ? "Mode: DOWNSTREAM\n" : "DOWNSTREAM phase\n");
        down_res = do_recv(sock, buffer, duration);
    }

    /*
     * RESULT
     */
    printf("\nServer IP   : %s\n", ip);
    printf("Port        : %d\n",   port);
    printf("Duration    : %d sec\n", duration);

    if (mode == MODE_UP)
    {
        print_result("UPSTREAM", "Sent", up_res.bytes, up_res.elapsed);
    }
    else if (mode == MODE_DOWN)
    {
        print_result("DOWNSTREAM", "Recv", down_res.bytes, down_res.elapsed);
    }
    else if (mode == MODE_BOTH)
    {
        print_result("UPSTREAM",   "Sent", up_res.bytes,   up_res.elapsed);
        print_result("DOWNSTREAM", "Recv", down_res.bytes, down_res.elapsed);

        long long total   = up_res.bytes + down_res.bytes;
        double    elapsed = up_res.elapsed + down_res.elapsed;
        printf("\n===== TOTAL =====\n");
        printf("Bytes Total : %lld\n",    total);
        printf("MB Total    : %.2f MB\n", total / 1024.0 / 1024.0);
        printf("Elapsed     : %.2f sec\n", elapsed);
        printf("Avg MB/s    : %.2f\n",   (total / 1024.0 / 1024.0) / elapsed);
    }

    close(sock);
    return 0;
}