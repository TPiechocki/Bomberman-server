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
#include <src/receiver/receiver.h>
#include <src/game/bomb.h>

void *broadcast(void *number_players) {
    int number = (int)number_players;
    list_t * temp;

    char buffer[2048];
    memset(buffer, 0, 2048);

    int msec = 0, trigger = 10; /* 10ms */
    clock_t before, delta;

    pthread_mutex_lock(&players_mutex);
    while (list_length(&players_root) < number) {
        pthread_mutex_unlock(&players_mutex);
        usleep(1000 * 100);
        printf("%d\n" ,list_length(&players_root));
        pthread_mutex_lock(&players_mutex);
    }
    pthread_mutex_unlock(&players_mutex);


    sprintf(buffer, "%d %d\n",  start_msg, number);
    temp = &players_root;
    int counter = 0;
    while (temp->next != NULL) {
        temp = temp->next;
        player_t *content = temp->content;
        // counter describe start position when x and y are 0s
        sprintf(buffer, "%s%d %s %d %d\n", buffer, counter++, content->name, 0, 0);
    }
    pthread_mutex_unlock(&players_mutex);


    temp = &sockets_root;
    pthread_mutex_lock(&sockets_mutex);
    while (temp->next != NULL) {
        temp = temp->next;
        write((int) temp->content, buffer, strlen(buffer));
    }
    pthread_mutex_unlock(&sockets_mutex);

    int tick = 0;
    while (1) {
        memset(buffer, 0, 2048);
        before = clock();
        //printf("START BROADCAST\n");
        temp = &players_root;

        pthread_mutex_lock(&players_mutex);
        sprintf(buffer, "%d %d %d\n",  players_msg, list_length(&players_root), tick++);
        while (temp->next != NULL) {
            temp = temp->next;
            player_t *content = temp->content;
            sprintf(buffer, "%s%s %d %d\n", buffer, content->name, content->x, content->y);
        }
        pthread_mutex_unlock(&players_mutex);
        pthread_mutex_lock(&broadcaster_mutex);
        CURRENT_TICK = tick;
        pthread_mutex_unlock(&broadcaster_mutex);

        temp = &bombs_root;
        pthread_mutex_lock(&bombs_mutex);
        sprintf(buffer, "%s%d %d\n",  buffer, bombs_msg, list_length(&bombs_root));
        while (temp->next != NULL) {
            temp = temp->next;
            bomb_t *content = temp->content;
            sprintf(buffer, "%s%s %d %d\n", buffer, content->name, content->tile, content->end_of_life);

            if (tick >= content->end_of_life) {
                list_t *backup = temp->next;
                list_remove(&bombs_root, temp->content);
                temp = backup;
                if (backup == NULL)
                    break;
            }
        }
        pthread_mutex_unlock(&bombs_mutex);

        temp = &sockets_root;
        pthread_mutex_lock(&sockets_mutex);
        while (temp->next != NULL) {
            temp = temp->next;
            write((int) temp->content, buffer, strlen(buffer));
        }
        pthread_mutex_unlock(&sockets_mutex);
        int test = 0;
        do {
            delta = clock() - before;
            msec = delta * 1000 / CLOCKS_PER_SEC;
            if (test == 0) {
                //printf("%d\n", msec);
                test = 1;
            }
        } while (msec < TICK);
    }
}