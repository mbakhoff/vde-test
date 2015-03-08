#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "vde-io.h"


static const size_t BUFFER_SIZE = 4096;

int vde_connect(struct vde_node *v, char *vde_switch) {
    VDECONN *conn = vde_open(vde_switch, "f2f_plug", NULL);
    if (!conn) {
        return 1;
    }
    v->write_buf.data = NULL;
    v->conn = conn;
    v->fd = vde_datafd(conn);
    snprintf(v->description, sizeof(v->description), "%s#%d", vde_switch, v->fd);
    return 0;
}

int vde_read(struct buffer_node *node, VDECONN *conn) {
    char *buf = malloc(BUFFER_SIZE);
    ssize_t len = vde_recv(conn, buf, BUFFER_SIZE, 0);
    if (len <= 0) {
        free(buf);
        return 1;
    }
    node->data = buf;
    node->len = (size_t) len;
    node->pos = 0;
    return 0;
}

int vde_write(struct buffer_node *buf, VDECONN *conn) {
    ssize_t result = vde_send(conn, buf->data + buf->pos, buf->len - buf->pos, 0);
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