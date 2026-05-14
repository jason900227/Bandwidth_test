#ifndef BW_COMMON_H
#define BW_COMMON_H

#include <stdint.h>

/* Network settings */
#define DEFAULT_PORT  5001
#define BUF_SIZE      65536
#define HANDSHAKE_KEY 0x12345678

/* Test mode */
#define MODE_UP   1
#define MODE_DOWN 2
#define MODE_BOTH 3

/* Handshake packet sent from client to server */
typedef struct
{
    uint32_t key;       /* Must match HANDSHAKE_KEY */
    uint32_t mode;      /* MODE_UP / MODE_DOWN / MODE_BOTH */
    uint32_t duration;  /* Test duration in seconds */
} control_t;

/* Test result returned by do_send / do_recv */
typedef struct
{
    long long bytes;    /* Total bytes transferred */
    double    elapsed;  /* Elapsed time in seconds */
} result_t;

/* Utility */
double    now_sec(void);
void      print_interval(double last, double now, double start, long long bytes);
void      print_result(const char *label, const char *dir, long long bytes, double elapsed);

/* Bandwidth test */
result_t  do_send(int fd, char *buf, unsigned dur);
result_t  do_recv(int fd, char *buf, unsigned dur);

#endif /* BW_COMMON_H */