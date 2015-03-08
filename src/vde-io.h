#ifndef F2F_VDE_H
#define F2F_VDE_H

#include <libvdeplug.h>
#include "common.h"

struct vde_node {
    struct buffer_node write_buf;
    char description[128];
    VDECONN *conn;
    int fd;
};

int vde_connect(struct vde_node *v, char *vde_switch);

int vde_read(struct buffer_node *node, VDECONN *conn);

int vde_write(struct buffer_node *buf, VDECONN *conn);

#endif
