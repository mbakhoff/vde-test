#include <stdlib.h>
#include <stdio.h>

#include <sys/poll.h>

#include "channel.h"


int handle_incoming_connections(struct channel **channels, int fd, char *vde_switch) {
    struct pollfd p;
    p.fd = fd;
    p.events = POLLIN;
    if (poll(&p, 1, 0) < 0) {
        return 1;
    }
    if ((p.revents & POLLIN) == 0) {
        return 0;
    }

    struct channel *ch = malloc(sizeof(struct channel));
    if (socket_accept(&ch->socket, fd)) {
        free(ch);
        return 1;
    }
    if (vde_connect(&ch->vde, vde_switch)) {
        free(ch);
        return 1;
    }
    ch->active = 1;

    printf("accepted connection from %s\n", ch->socket.description);

    ch->next = *channels;
    *channels = ch;
    return 0;
}

void register_fds(struct channel *channel, struct pollfd *socket, struct pollfd *vde) {
    if (!channel->active) {
        socket->fd = -1;
        vde->fd = -1;
        return;
    }

    socket->fd = channel->socket.fd;
    socket->events = 0;
    vde->fd = channel->vde.fd;
    vde->events = 0;

    if (channel->socket.write_buf.data != NULL) {
        socket->events |= POLLOUT;
    } else {
        vde->events |= POLLIN;
    }
    if (channel->vde.write_buf.data != NULL) {
        vde->events |= POLLOUT;
    } else {
        socket->events |= POLLIN;
    }
}

void handle_channel(struct channel *channel, struct pollfd *s_poll, struct pollfd *v_poll) {
    if (!channel->active) {
        return;
    }

    struct socket_node *socket = &channel->socket;
    struct vde_node *vde = &channel->vde;

    if ((s_poll->revents & (POLLHUP | POLLERR)) != 0 || (v_poll->revents & (POLLHUP | POLLERR)) != 0) {
        fprintf(stderr, "error in channel %s <-> %s\n", socket->description, vde->description);
        channel->active = 0;
    }

    int socket_idle = channel->socket.write_buf.data == NULL;
    int vde_idle = channel->vde.write_buf.data == NULL;

    if (!socket_idle && (s_poll->revents & POLLOUT) != 0) {
        if (socket_write(&socket->write_buf, socket->fd)) {
            fprintf(stderr, "error writing to socket %s\n", socket->description);
            channel->active = 0;
        }
    }
    if (!vde_idle && (v_poll->revents & POLLOUT) != 0) {
        if (vde_write(&vde->write_buf, vde->conn)) {
            fprintf(stderr, "error writing to vde %s\n", vde->description);
            channel->active = 0;
        }
    }
    if (socket_idle && (v_poll->revents & POLLIN) != 0) {
        if (vde_read(&socket->write_buf, vde->conn)) {
            fprintf(stderr, "error reading from vde %s\n", vde->description);
            channel->active = 0;
        }
    }
    if (vde_idle && (s_poll->revents & POLLIN) != 0) {
        if (socket_read(&vde->write_buf, socket->fd)) {
            fprintf(stderr, "error reading from socket %s\n", socket->description);
            channel->active = 0;
        }
    }
}

int handle_channels(struct channel *channels) {
    struct pollfd fds[32];
    nfds_t channel_count;

    channel_count = 0;
    for (struct channel *ch = channels; ch != NULL; ch = ch->next) {
        register_fds(ch, &fds[channel_count * 2], &fds[channel_count * 2 + 1]);
        channel_count++;
    }

    int events = poll((struct pollfd*) &fds, channel_count * 2, 200);
    if (events < 0) {
        return 1;
    }

    channel_count = 0;
    for (struct channel *ch = channels; ch != NULL; ch = ch->next) {
        handle_channel(ch, &fds[channel_count * 2], &fds[channel_count * 2 + 1]);
        channel_count++;
    }

    return 0;
}

struct channel *channel_create(const char *address, const char *port, char *vde_switch) {
    struct channel *ch = malloc(sizeof(struct channel));
    if (socket_connect(&ch->socket, address, port)) {
        fprintf(stderr, "failed to connect to remote %s:%s\n", address, port);
        free(ch);
        return NULL;
    }
    if (vde_connect(&ch->vde, vde_switch)) {
        fprintf(stderr, "failed to connect to vde switch %s\n", vde_switch);
        free(ch);
        return NULL;
    }

    ch->active = 1;
    return ch;
}
