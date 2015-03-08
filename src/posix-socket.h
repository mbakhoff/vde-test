#ifndef F2F_SOCKET_H
#define F2F_SOCKET_H

#include "common.h"

struct socket_node {
    struct buffer_node write_buf;
    char description[24];
    int fd;
};

int socket_connect(struct socket_node *s, const char *address, const char *port);

int socket_listen(const char *address, const char *port);

int socket_accept(struct socket_node *s, int fd);

int socket_read(struct buffer_node *node, int fd);

int socket_write(struct buffer_node *buf, int fd);

#endif
