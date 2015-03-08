#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "posix-socket.h"


static const size_t BUFFER_SIZE = 4096;

static int parse_uint16(const char *str, uint16_t *result) {
    errno = 0;
    long port = strtol(str, NULL, 10);
    if (errno || port > UINT16_MAX || port < 0) {
        return 1;
    }
    *result = (uint16_t) port;
    return 0;
}

void socket_set(struct socket_node *s, struct sockaddr_in *binding, int fd) {
    s->write_buf.data = NULL;
    s->fd = fd;

    char *address_string = inet_ntoa(binding->sin_addr);
    uint16_t port = ntohs(binding->sin_port);
    snprintf(s->description, sizeof(s->description), "%s:%d", address_string, port);
}

int parse_ipv4(struct sockaddr_in *result, const char *address_str, const char *port_str) {
    struct in_addr address;
    if (inet_pton(AF_INET, address_str, &address) != 1) {
        return 1;
    }
    uint16_t port;
    if (parse_uint16(port_str, &port)) {
        return 1;
    }
    result->sin_family = AF_INET;
    result->sin_port = htons(port);
    result->sin_addr = address;
    return 0;
}

void set_reuse_addr(int fd) {
    int value = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(int))) {
        fprintf(stderr, "could not set SO_REUSEADDR on fd %d\n", fd);
    }
}

int socket_listen(const char *address, const char *port) {
    struct sockaddr_in binding;
    if (parse_ipv4(&binding, address, port)) {
        fprintf(stderr, "failed to parse %s:%s\n", address, port);
        return -1;
    }
    int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (fd < 0) {
        return -1;
    }
    set_reuse_addr(fd);
    if (bind(fd, (struct sockaddr *) &binding, sizeof(struct sockaddr_in))) {
        return -1;
    }
    if (listen(fd, 32)) {
        return -1;
    }
    return fd;
}

int socket_connect(struct socket_node *s, const char *address, const char *port) {
    struct sockaddr_in binding;
    if (parse_ipv4(&binding, address, port)) {
        fprintf(stderr, "failed to parse %s:%s\n", address, port);
        return 1;
    }
    int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (fd < 0) {
        return 1;
    }
    set_reuse_addr(fd);
    if (connect(fd, (struct sockaddr *) &binding, (socklen_t) sizeof(struct sockaddr_in))) {
        if (errno != EINPROGRESS) {
            return 1;
        }
        // else it's ok, connecting asynchronously
    }
    socket_set(s, &binding, fd);
    return 0;
}

int socket_accept(struct socket_node *s, int fd) {
    struct sockaddr info;
    socklen_t len = sizeof(struct sockaddr);
    int accepted = accept(fd, &info, &len);
    if (accepted == -1 || info.sa_family != AF_INET) {
        return 1;
    }
    struct sockaddr_in *ipv4 = (struct sockaddr_in *) &info;
    socket_set(s, ipv4, accepted);
    return 0;
}

int socket_read(struct buffer_node *node, int fd) {
    char *buf = malloc(BUFFER_SIZE);
    ssize_t len = read(fd, buf, BUFFER_SIZE);
    if (len <= 0) {
        free(buf);
        return 1;
    }
    node->data = buf;
    node->len = (size_t) len;
    node->pos = 0;
    return 0;
}

int socket_write(struct buffer_node *buf, int fd) {
    ssize_t result = write(fd, buf->data + buf->pos, buf->len - buf->pos);
    if (result < 0) {
        return 1;
    }
    buf->pos += result;
    if (buf->pos == buf->len) {
        free(buf->data);
        buf->data = NULL;
    }
    return 0;
}

