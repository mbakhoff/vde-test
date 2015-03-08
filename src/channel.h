#ifndef F2F_CHANNEL_H
#define F2F_CHANNEL_H

#include "posix-socket.h"
#include "vde-io.h"

/** pair of tcp socket and vde plug */
struct channel {
    struct channel *next;
    struct socket_node socket;
    struct vde_node vde;
    int active;
};

int handle_channels(struct channel *channels);

int handle_incoming_connections(struct channel **channels, int fd, char *vde_switch);

struct channel *channel_create(const char *address, const char *port, char *vde_switch);

#endif