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
#include <src/game/bomb.h>
#include <src/sender/broadcaster.h>
#include <src/game/blocks.h>

void *
connection_handler(void *args) {

    /* Get the socket descriptor */
    receiver_args_t *receiver_args = (receiver_args_t *)args;
    int sock = receiver_args->sock;
    ssize_t read_size;
    char *message , nick[65];

    player_t *this_player;

    char buffer[2048];
    char buffer_send[2048];
    do {
        read_size = recv(sock , buffer , 2048 , 0);
        buffer[read_size] = '\0';

        char *buff_ptr = buffer;

        int buff_length;  // will store number of bytes read by current command from client

        int x, y;
        int action_counter;
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

                if (state == 0 && list_length(&players_root) < receiver_args->max_players) {       // create new player
                    this_player = (player_t *) malloc(sizeof(player_t));
                    strcpy(this_player->name, nick);
                    this_player->connected = 1;
                    this_player->alive = 1;
                    list_append(&players_root, (void *)this_player);
                    players_list_display(&players_root);
                    pthread_mutex_unlock(&players_mutex);
                    // add to listeners
                    pthread_mutex_lock(&sockets_mutex);
                    list_append(&sockets_root, (void *)sock);
                    list_display(&sockets_root);
                    pthread_mutex_unlock(&sockets_mutex);
                } else if (state != -1 && state != 0){     // reconnected player
                    this_player = (player_t *)state;

                    // send start signal to the reconnected client
                    sprintf(buffer_send  , "%d %d\n",  start_msg, list_length(&players_root));
                    list_t *temp = &players_root;
                    int counter = 0;
                    while (temp->next != NULL) {
                        temp = temp->next;
                        player_t *content = temp->content;
                        sprintf(buffer_send, "%s%d %s %d %d\n", buffer_send, counter++, content->name, content->x, content->y);
                    }
                    pthread_mutex_unlock(&players_mutex);
                    pthread_mutex_lock(&blocks_mutex);
                    sprintf(buffer_send, "%s%d\n", buffer_send, walls_msg);
                    for (int i = 0; i < 11; ++i) {
                        for (int j = 0; j < 11; ++j) {
                            sprintf(buffer_send, "%s%d ", buffer_send, blocks[i][j]);
                        }
                    }
                    pthread_mutex_unlock(&blocks_mutex);
                    write(sock, buffer_send, strlen(buffer_send));
                    // Add socket to the list
                    pthread_mutex_lock(&sockets_mutex);
                    list_append(&sockets_root, (void *)sock);
                    list_display(&sockets_root);
                    pthread_mutex_unlock(&sockets_mutex);
                } else if (state == -1 || list_length(&players_root) >= receiver_args->max_players) {   // player couldn't be connected
                    pthread_mutex_unlock(&players_mutex);
                    // remove invalid socket
                    pthread_mutex_lock(&sockets_mutex);
                    list_remove(&sockets_root, (void *)(int64_t)sock);
                    list_display(&sockets_root);
                    pthread_mutex_unlock(&sockets_mutex);
                    close(sock);
                    pthread_exit(NULL);
                }
            }
            else if (this_player != NULL && msg_type == move_msg) {
                sscanf(buff_ptr, "%s %d %d %d\n%n", nick, &x, &y, &action_counter, &buff_length);
                buff_ptr += buff_length;
                //printf("Client move: %s %d %d\n", nick, x, y);
                this_player->x = x;
                this_player->y = y;
            }
            else if (this_player != NULL && msg_type == bomb_msg) {
                int tile;
                sscanf(buff_ptr, "%s %d\n%n", nick, &tile, &buff_length);
                buff_ptr += buff_length;
                printf("Bomb placed: %s %d\n", nick, tile);
                bomb_t *this_bomb = (bomb_t *)malloc(sizeof(bomb_t));
                strcpy(this_bomb->name, nick);
                this_bomb->tile = tile;
                pthread_mutex_lock(&broadcaster_mutex);
                this_bomb->start_of_explostion = CURRENT_TICK + BOMB_TICKS;
                this_bomb->end_of_life = CURRENT_TICK + BOMB_TICKS + LAVA_TICKS;
                pthread_mutex_unlock(&broadcaster_mutex);
                pthread_mutex_lock(&bombs_mutex);
                list_append(&bombs_root, (void *)this_bomb);
                pthread_mutex_unlock(&bombs_mutex);
            }
        }

        /* Clear the message buffer */
        memset(buffer, 0, 2048);
    } while(read_size > 0); /* Wait for empty line */

    fprintf(stderr, "Client disconnected\n");

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
