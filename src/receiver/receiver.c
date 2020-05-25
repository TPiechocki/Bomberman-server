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

    // Cast arguments to proper types
    receiver_args_t *receiver_args = (receiver_args_t *)args;
    int sock = receiver_args->sock;
    ssize_t read_size;
    char nick[65];

    player_t *this_player;

    char buffer[2048];          // buffer for received messages
    char buffer_send[2048];     // buffer for sending messages
    do {
        // read new message
        read_size = recv(sock , buffer , 2048 , 0);
        buffer[read_size] = '\0';

        char *buff_ptr = buffer;    // pointer to current position of the buffer

        int buff_length;  // will store number of bytes read by current command from client

        int x, y;
        int action_counter;
        MSG msg_type;
        while (*buff_ptr) {
            // process type of the message
            sscanf(buff_ptr, "%d%n", &msg_type, &buff_length);
            buff_ptr += buff_length;        // move buff_ptr to the next characters

            // new connection handling
            if (msg_type == name_msg) {
                sscanf(buff_ptr, "%s%n", nick, &buff_length);
                buff_ptr += buff_length;
                printf("Connection with nick: %s\n", nick);

                pthread_mutex_lock(&players_mutex);
                int64_t state = players_check_existence(&players_root, nick);

                // new player
                if (state == 0 && list_length(&players_root) < receiver_args->max_players) {
                    // create the new player struct
                    this_player = (player_t *) malloc(sizeof(player_t));
                    strcpy(this_player->name, nick);
                    this_player->connected = 1;
                    this_player->alive = 1;
                    list_append(&players_root, (void *)this_player);
                    pthread_mutex_unlock(&players_mutex);
                    // add to listeners
                    pthread_mutex_lock(&sockets_mutex);
                    list_append(&sockets_root, (void *)sock);
                    list_display(&sockets_root);
                    pthread_mutex_unlock(&sockets_mutex);
                }
                // reconnection of the player who got disconnected earlier (nick must remain the same)
                else if (state != -1 && state != 0) {
                    this_player = (player_t *)state;

                    // send start signal to the reconnected client so he can get actual full state of the game
                    sprintf(buffer_send  , "%d %d\n",  start_msg, list_length(&players_root));
                    // send list of all the current players
                    list_t *temp = &players_root;
                    int counter = 0;
                    while (temp->next != NULL) {
                        temp = temp->next;
                        player_t *content = temp->content;
                        sprintf(buffer_send, "%s%d %s %d %d\n", buffer_send, counter++,
                                content->name, content->x, content->y);
                    }
                    pthread_mutex_unlock(&players_mutex);
                    // send actual positions of breakable blocks
                    pthread_mutex_lock(&blocks_mutex);
                    sprintf(buffer_send, "%s%d\n", buffer_send, walls_msg);
                    for (int i = 0; i < 11; ++i) {
                        for (int j = 0; j < 11; ++j) {
                            sprintf(buffer_send, "%s%d ", buffer_send, blocks[i][j]);
                        }
                    }
                    pthread_mutex_unlock(&blocks_mutex);
                    write(sock, buffer_send, strlen(buffer_send));  // send the created message
                    // Add socket to the list
                    pthread_mutex_lock(&sockets_mutex);
                    list_append(&sockets_root, (void *)sock);
                    list_display(&sockets_root);
                    pthread_mutex_unlock(&sockets_mutex);
                }
                // player couldn't be connected, because limit of the players is fulfilled or the player with the same
                // nick is already connected
                else if (state == -1 || list_length(&players_root) >= receiver_args->max_players) {
                    pthread_mutex_unlock(&players_mutex);
                    // remove socket of the player who is invalid
                    pthread_mutex_lock(&sockets_mutex);
                    list_remove(&sockets_root, (void *)(int64_t)sock);
                    list_display(&sockets_root);
                    pthread_mutex_unlock(&sockets_mutex);
                    close(sock);
                    pthread_exit(NULL);
                }
            }

            // Movement of the player
            else if (this_player != NULL && msg_type == move_msg) {
                sscanf(buff_ptr, "%s %d %d %d\n%n", nick, &x, &y, &action_counter, &buff_length);
                buff_ptr += buff_length;
                // update player's position
                this_player->x = x;
                this_player->y = y;
            }

            // Player place the bomb
            else if (this_player != NULL && msg_type == bomb_msg) {
                int tile;       // number of the tile where the bomb is placed
                sscanf(buff_ptr, "%s %d\n%n", nick, &tile, &buff_length);
                buff_ptr += buff_length;
                // create a new bomb
                bomb_t *this_bomb = (bomb_t *)malloc(sizeof(bomb_t));
                strcpy(this_bomb->name, nick);
                this_bomb->tile = tile;
                pthread_mutex_lock(&broadcaster_mutex);
                this_bomb->start_of_explosion = CURRENT_TICK + BOMB_TICKS;     // tick when the explosion will start
                this_bomb->end_of_life = CURRENT_TICK + BOMB_TICKS + LAVA_TICKS;    // tick of the end of the bomb
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
