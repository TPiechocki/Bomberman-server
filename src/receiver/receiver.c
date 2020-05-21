//
// Created by Tomasz Piechocki on 18/05/2020.
//

#include "receiver.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <sys/socket.h>
#include <pthread.h>
#include <src/structures/list.h>
#include <src/game/player.h>

void *
connection_handler(void *socket_desc) {

    /* Get the socket descriptor */
    int sock = * (int64_t *)socket_desc;
    ssize_t read_size;
    char *message , nick[65];

    player_t *this_player;

    char buffer[2048];
    do {
        read_size = recv(sock , buffer , 2000 , 0);
        buffer[read_size] = '\0';

        char *buff_ptr = buffer;

        int buff_length;  // will store number of bytes read by current command from client

        int x, y, length;
        int counter;
        MSG msg_type;
        while (*buff_ptr) {
            sscanf(buff_ptr, "%d%n", &msg_type, &buff_length);
            buff_ptr += buff_length;
            if (msg_type == name_msg) {
                sscanf(buff_ptr, "%s%n", nick, &buff_length);
                buff_ptr += buff_length;
                printf("Client message: %s\n", nick);

                pthread_mutex_lock(&players_mutex);
                int64_t state = players_check_existence(&players_root, nick);

                if (state == 0) {       // create new player
                    this_player = (player_t *) malloc(sizeof(player_t));
                    strcpy(this_player->name, nick);
                    this_player->connected = 1;
                    list_append(&players_root, (void *)this_player);
                    players_list_display(&players_root);
                    pthread_mutex_unlock(&players_mutex);
                } else if (state == -1) {   // player couldn't be connected
                    pthread_mutex_unlock(&players_mutex);
                    close(sock);
                    pthread_exit(NULL);
                } else {     // reconnected player
                    this_player = (player_t *)state;
                    pthread_mutex_unlock(&players_mutex);
                }
            }
            else if (this_player != NULL && msg_type == move_msg) {
                sscanf(buff_ptr, "%s %d %d %d\n%n", nick, &x, &y, &counter, &buff_length);
                buff_ptr += buff_length;
                //printf("Client move: %s %d %d\n", nick, x, y);
                this_player->x = x;
                this_player->y = y;
            }
        }

        /* Clear the message buffer */
        memset(buffer, 0, 2048);
    } while(read_size > 0); /* Wait for empty line */

    fprintf(stderr, "Client disconnected\n");

    // TODO decide when to remove player completely
    pthread_mutex_lock(&players_mutex);
    player_disconnect(&players_root, nick);
    players_list_display(&players_root);
    pthread_mutex_unlock(&players_mutex);

    // Remove socket when disconnected
    pthread_mutex_lock(&sockets_mutex);
    list_remove(&sockets_root, (void *)(int64_t)sock);
    list_display(&sockets_root);
    pthread_mutex_unlock(&sockets_mutex);

    close(sock);
    pthread_exit(NULL);
}
