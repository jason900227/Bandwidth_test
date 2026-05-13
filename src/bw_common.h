#ifndef BW_COMMON_H
#define BW_COMMON_H

#include <stdint.h>

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

double    now_sec(void);
void      print_interval(double last, double now, double start, long long bytes);
void      print_result(const char *label, const char *dir, long long bytes, double elapsed);
result_t  do_send(int fd, char *buf, unsigned dur);
result_t  do_recv(int fd, char *buf, unsigned dur);

#endif /* BW_COMMON_H */