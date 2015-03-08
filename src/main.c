
#include <stdio.h>
#include <errno.h>

#include <unistd.h>
#include <sys/stat.h>

#include "channel.h"


int execvp_vde_switch(char *switch_path) {
    char mgmt[128];
    snprintf((char*)&mgmt, sizeof(mgmt), "%s-mgmt", switch_path);

    char *args[] = {"vde_switch", "-d", "--sock", switch_path, "--mgmt", mgmt, NULL};
    if (execvp("vde_switch", args)) {
        fprintf(stderr, "failed to start vde_switch at %s (%d)\n", switch_path, errno);
        return 1;
    }
    return 0;
}

void wait_vde_ctl(char *vde_switch) {
    struct stat st;
    char path[128];
    snprintf((char*)&path, sizeof(path), "%s/ctl", vde_switch);
    while (stat(path, &st)) {
        sleep(1);
    }
}

int init_channels(struct channel **channels, int argc, char **argv, char *vde_switch) {
    int argv_peers = (argc - 3) / 2;
    for (int i = 0; i < argv_peers; i++) {
        struct channel *ch = channel_create(argv[3 + i*2], argv[4 + i*2], vde_switch);
        if (ch == NULL) {
            return 1;
        }
        ch->next = *channels;
        *channels = ch;
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("usage: vde-test <vde-switch-path> <server-port> [peer-ip peer-port] .. \n");
        return 1;
    }

    char *vde_switch = argv[1];
    char *argv_port = argv[2];

    pid_t pid = fork();
    if (pid == 0) {
        return execvp_vde_switch(vde_switch);
    } else {
        // TODO find a way to kill that sucker on exit
        printf("don't forget to kill the switch\n");
        wait_vde_ctl(vde_switch);
    }

    int server_fd = socket_listen("0.0.0.0", argv_port);
    if (server_fd < 0) {
        fprintf(stderr, "failed to open server socket\n");
        return 1;
    }

    struct channel *channels = NULL;
    if (init_channels(&channels, argc, argv, vde_switch)) {
        return 1;
    }

    printf("starting loop\n");
    while (1) {
        if (handle_incoming_connections(&channels, server_fd, vde_switch)) {
            fprintf(stderr, "error in server socket, exiting\n");
            return 1;
        }
        if (handle_channels(channels)) {
            fprintf(stderr, "error in channels, exiting\n");
            return 1;
        }
    }

    // ctrl+c
}
