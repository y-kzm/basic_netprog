 /**
 * @file server.c
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
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_SOCK 20

int main(int argc, char **argv)
{
    struct addrinfo hints, *res, *res0;
    int error;
    struct sockaddr_storage from;
    socklen_t fromlen;
    int ls;
    int s[MAX_SOCK];
    int smax;
    int sockmax;
    fd_set rfd, rfd0;
    int n;
    int i;
    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
    const int on = 1; // IPv4-Mapped IPv6 Addressでの接続を無効化

    if (argc != 2) {
        fprintf(stderr, "usage: ./server port_num\n");
        exit(EXIT_FAILURE);
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    error = getaddrinfo(NULL, argv[1], &hints, &res0);
    if (error) {
        fprintf(stderr, "%s: %s\n", argv[1], gai_strerror(error));
        exit(EXIT_FAILURE);
    }
    
    smax = 0;
    sockmax = -1;
    for (res = res0; res && smax < MAX_SOCK; res = res->ai_next) {
        s[smax] = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (s[smax] < 0) {
            continue;
        }

        if (s[smax] >= FD_SETSIZE) {
            close(s[smax]);
            s[smax] = -1;
            continue;
        }

        if (res->ai_family == AF_INET6 && 
            setsockopt(s[smax], IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof(on)) < 0) {
                perror("setsockopt");
                s[smax] = -1;
                continue;
            }
        
        if (bind(s[smax], res->ai_addr, res->ai_addrlen) < 0) {
            close(s[smax]);
            s[smax] = -1;
            continue;
        }

        if (listen(s[smax], 5) < 0) {
            close(s[smax]);
            s[smax] = -1;
            continue;
        }

        error = getnameinfo(res->ai_addr, res->ai_addrlen, 
                            hbuf, sizeof(hbuf), sbuf, sizeof(sbuf),
                            NI_NUMERICHOST | NI_NUMERICSERV);
        if (error) {
            fprintf(stderr, "./server: %s\n", gai_strerror(error));
            exit(EXIT_FAILURE);
        }
        fprintf(stderr, "listen to %s %s\n", hbuf, sbuf);

        if (s[smax] > sockmax) {
            sockmax = s[smax];
        }
        smax++;
    }

    if (smax == 0) {
        fprintf(stderr, "./server: no socket to listen to\n");
        exit(EXIT_FAILURE);
    }

    FD_ZERO(&rfd0);
    for (i = 0; i < smax; i ++) {
        FD_SET(s[i], &rfd0);
    }

    while (1) {
        rfd = rfd0;
        n = select(sockmax + 1, &rfd, NULL, NULL, NULL);
        if (n < 0) {
            perror("select");
            exit(EXIT_FAILURE);
        }
        for (i = 0; i < smax; i++) {
            if (FD_ISSET(s[i], &rfd)) {
                fromlen = sizeof(from);
                ls = accept(s[i], (struct sockaddr *)&from, &fromlen);
                if (ls < 0) {
                    continue;
                }
                error = getnameinfo((struct sockaddr *)&from, &fromlen,
                            hbuf, sizeof(hbuf), sbuf, sizeof(sbuf),
                            NI_NUMERICHOST | NI_NUMERICSERV);
                if (error) {
                    fprintf(stderr, "./server: %s\n", gai_strerror(error));
                    exit(EXIT_FAILURE);
                }
                fprintf(stderr, "connection from: %s %s\n", hbuf, sbuf);

                write(ls, "Hello\n", 6);
                close(ls);
            }
        }
    }
    
}