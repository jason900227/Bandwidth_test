#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "bw_common.h"

void usage(const char *prog)
{
    printf("\nUsage:\n");
    printf("  %s [-p port]\n", prog);
    printf("\nOptions:\n");
    printf("  -p <port>    TCP port (default: %d)\n", DEFAULT_PORT);
    printf("  -h, --help   Show this help message\n\n");
}

int main(int argc, char *argv[])
{
    int server_fd;
    int client_fd;
    int port = DEFAULT_PORT;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-p") == 0 && i + 1 < argc)
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
            printf("Unknown option: %s\n", argv[i]);
            usage(argv[0]);
            return -1;
        }
    }

    struct sockaddr_in addr;
    char buffer[BUF_SIZE];

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); return -1; }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        close(server_fd);
        return -1;
    }

    listen(server_fd, 1);
    printf("Server listening on %d...\n", port);

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

    if (ctrl.key != HANDSHAKE_KEY)
    {
        printf("Bad protocol\n");
        close(client_fd);
        close(server_fd);
        return -1;
    }

    const char *mode_names[] = { "", "UPSTREAM", "DOWNSTREAM", "BOTH" };
    printf("Mode        : %s\n",       mode_names[ctrl.mode]);
    printf("Duration    : %u sec\n\n", ctrl.duration);

    result_t up_res   = { 0, 0.0 };
    result_t down_res = { 0, 0.0 };

    memset(buffer, 'A', BUF_SIZE);

    /*
     * UPSTREAM
     */
    if (ctrl.mode == MODE_UP || ctrl.mode == MODE_BOTH)
    {
        if (ctrl.mode == MODE_BOTH)
            printf("UPSTREAM phase\n");

        up_res = do_recv(client_fd, buffer, ctrl.duration);
        print_result("UPSTREAM", "Recv", up_res.bytes, up_res.elapsed);
    }

    /*
     * DOWNSTREAM
     */
    if (ctrl.mode == MODE_DOWN || ctrl.mode == MODE_BOTH)
    {
        if (ctrl.mode == MODE_BOTH)
            printf("\nDOWNSTREAM phase\n");

        down_res = do_send(client_fd, buffer, ctrl.duration);
        shutdown(client_fd, SHUT_WR);
        print_result("DOWNSTREAM", "Sent", down_res.bytes, down_res.elapsed);
    }

    close(client_fd);
    close(server_fd);
    return 0;
}