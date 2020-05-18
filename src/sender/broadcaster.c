//
// Created by Tomasz Piechocki on 18/05/2020.
//

#include "broadcaster.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <sys/socket.h>
#include <pthread.h>
#include <src/structures/list.h>
#include <src/game/player.h>

void *broadcast() {
    char buffer[2048];
    strcpy(buffer, "");

    int msec = 0, trigger = 10; /* 10ms */
    clock_t before, delta;


    while (1) {
        before = clock();
        printf("START BROADCAST\n");
        list_t *temp = &players_root;

        pthread_mutex_lock(&players_mutex);
        while (temp->next != NULL) {
            temp = temp->next;
            player_t *content = temp->content;
            sprintf(buffer, "%s %s %d %d", buffer, content->name, content->x, content->y);
        }
        pthread_mutex_unlock(&players_mutex);

        temp = &sockets_root;
        pthread_mutex_lock(&sockets_mutex);
        while (temp->next != NULL) {
            temp = temp->next;
            write((int) temp->content, buffer, strlen(buffer));
        }
        pthread_mutex_unlock(&sockets_mutex);

        do {
            delta = clock() - before;
            msec = delta * 1000 / CLOCKS_PER_SEC;
            //printf("%d\n", msec);
        } while (msec < TICK);
    }
}