/**
 * @file client.c
 * @version 0.1
 * @date 2023-05-21
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main(int argc, char **argv)
{
    struct addrinfo hints, *res, *res0;
    ssize_t l;
    int s;
    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV]; // NI_MAXHOST 1025, NI_MAXSERV 32 
    char buf[1024];
    int error;

    if (argc != 3) {
        fprintf(stderr, "usage: ./client host_name port_num\n");
        exit(EXIT_FAILURE);
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    error = getaddrinfo(argv[1], argv[2], &hints, &res0);
    if (error) {
        fprintf(stderr, "%s %s: %s\n",argv[1], argv[2], gai_strerror(error));
        exit(EXIT_FAILURE);
    }

    for (res = res0; res; res = res->ai_next) {
        error = getnameinfo(res->ai_addr, res->ai_addrlen, 
                            hbuf, sizeof(hbuf), sbuf, sizeof(sbuf), 
                            NI_NUMERICHOST | NI_NUMERICSERV);
        if (error) {
            fprintf(stderr, "%s %s: %s\n", argv[1], argv[2], gai_strerror(error));
            continue;
        }
        fprintf(stderr, "trying %s port %s\n", hbuf, sbuf);

        s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (s < 0) {
            continue;
        }
        if (connect(s, res->ai_addr, res->ai_addrlen) < 0) {
            close(s);
            s = -1;
            continue;
        }

        while ((l = read(s, buf, sizeof(buf))) > 0) {
            write(STDOUT_FILENO, buf, l);
        }
        close(s);

        exit(EXIT_SUCCESS);
    }

    fprintf(stderr, "./client: no destination to cnnect to\n");
    exit(EXIT_FAILURE);
}