#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include "bw_common.h"

double now_sec(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1e6;
}

void print_interval(double last, double now,
                    double start, long long bytes)
{
    double bandwidth = (bytes / 1024.0 / 1024.0) / (now - last);
    printf("[%6.2f - %6.2f]   %10.2f\n",
           last - start, now - start, bandwidth);
}

void print_result(const char *label, const char *dir,
                  long long bytes, double elapsed)
{
    printf("\n===== %s RESULT =====\n", label);
    printf("Bytes %-5s : %lld\n",    dir, bytes);
    printf("MB    %-5s : %.2f MB\n", dir, bytes / 1024.0 / 1024.0);
    printf("Elapsed     : %.2f sec\n", elapsed);
    printf("Avg MB/s    : %.2f\n",  (bytes / 1024.0 / 1024.0) / elapsed);
}

result_t do_send(int fd, char *buf, unsigned dur)
{
    long long total_bytes    = 0;
    long long interval_bytes = 0;
    unsigned  printed        = 0;
    double    start          = now_sec();
    double    last           = start;
    ssize_t   n;

    printf("%-18s %-18s\n", "Interval(sec)", "Bandwidth(MB/s)");
    printf("------------------------------------------------\n");

    while (now_sec() - start < dur)
    {
        n = write(fd, buf, BUF_SIZE);

        if (n <= 0)
        {
            double elapsed = now_sec() - start;
            if (printed < dur)
                print_interval(last, now_sec(), start, interval_bytes);
            return (result_t){ total_bytes, elapsed };
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

    double elapsed = now_sec() - start;

    if (printed < dur)
        print_interval(last, now_sec(), start, interval_bytes);

    return (result_t){ total_bytes, elapsed };
}

result_t do_recv(int fd, char *buf, unsigned dur)
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

        if (now - last >= 1.0 && printed < dur)
        {
            print_interval(last, now, start, interval_bytes);
            printed++;
            interval_bytes = 0;
            last = now;
        }
    }

    double elapsed = now_sec() - start;

    if (interval_bytes > 0 && printed < dur)
        print_interval(last, now_sec(), start, interval_bytes);

    return (result_t){ total_bytes, elapsed };
}