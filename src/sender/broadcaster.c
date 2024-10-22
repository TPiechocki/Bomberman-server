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
#include <src/game/blocks.h>

void *broadcast(void *number_players) {
    int number = (int)number_players;
    list_t * temp;

    char buffer[2048];
    memset(buffer, 0, 2048);

    int msec = 0;
    clock_t before, delta;

    // Blocks which were destroyed in the current tick
    list_t destroyed_blocks;
    destroyed_blocks.next = NULL;

    // Loop waiting for all the players to connect at the start
    pthread_mutex_lock(&players_mutex);
    while (list_length(&players_root) < number) {
        pthread_mutex_unlock(&players_mutex);
        usleep(1000 * 100);
        pthread_mutex_lock(&players_mutex);
    }
    pthread_mutex_unlock(&players_mutex);

    // Message when number of the players is fulfilled and the game can start
    sprintf(buffer, "%d %d\n",  start_msg, number);
    temp = &players_root;
    int counter = 0;        // place on players' list which describe start position of the player
    // send list of all the players
    while (temp->next != NULL) {
        temp = temp->next;
        player_t *content = temp->content;
        // counter describe start position only when x and y are 0s
        sprintf(buffer, "%s%d %s %d %d\n", buffer, counter++, content->name, 0, 0);
    }
    pthread_mutex_unlock(&players_mutex);
    // send all the destroyable blocks
    pthread_mutex_lock(&blocks_mutex);
    sprintf(buffer, "%s%d\n", buffer, walls_msg);
    for (int i = 0; i < 11; ++i) {
        for (int j = 0; j < 11; ++j) {
            sprintf(buffer, "%s%d ", buffer, blocks[i][j]);
        }
    }
    pthread_mutex_unlock(&blocks_mutex);

    // send created start message to all the connected players
    temp = &sockets_root;
    pthread_mutex_lock(&sockets_mutex);
    while (temp->next != NULL) {
        temp = temp->next;
        write((int) temp->content, buffer, strlen(buffer));
    }
    pthread_mutex_unlock(&sockets_mutex);


    // Proper game broadcast based on 15 ms ticks
    int tick = 0;
    while (1) {
        memset(buffer, 0, 2048);
        before = clock();   // start counting the tick timer

        // first is list of the bombs with current tick number
        temp = &bombs_root;
        pthread_mutex_lock(&bombs_mutex);
        pthread_mutex_lock(&broadcaster_mutex);
        sprintf(buffer, "%d %d %d\n", bombs_msg, list_length(&bombs_root), tick++);     // increment tick
        CURRENT_TICK = tick;
        pthread_mutex_unlock(&broadcaster_mutex);

        // for each bomb
        while (temp->next != NULL) {
            temp = temp->next;
            bomb_t *content = temp->content;
            sprintf(buffer, "%s%s %d %d %d\n", buffer, content->name, content->tile,
                    content->start_of_explosion, content->end_of_life);    // append bomb to the message


            // destroy destroyable block in range of the bomb if bomb just exploded amd add them to the list which
            // will be used later to inform the clients
            if (tick == content->start_of_explosion) {
                pthread_mutex_lock(&blocks_mutex);
                int row = content->tile / 11;
                int column = content->tile % 11;

                for (int i = 1; i <= 2; ++i) {  // below
                    if (row + i <= 10) {
                        if (column % 2 == 1 && (row + i) % 2 == 1)    // unbreakable block
                            break;
                        if (blocks[row+i][column] == 1) {
                            blocks[row+i][column] = 0;
                            list_append(&destroyed_blocks, (void *)((row + i) * 11 + column));
                            break;
                        }
                    }
                }
                for (int i = 1; i <= 2; ++i) {  // above
                    if (row - i >= 0) {
                        if (column % 2 == 1 && (row - i) % 2 == 1)    // unbreakable block
                            break;
                        if (blocks[row-i][column] == 1) {
                            blocks[row-i][column] = 0;
                            list_append(&destroyed_blocks, (void *)((row - i) * 11 + column));
                            break;
                        }
                    }
                }
                for (int i = 1; i <= 2; ++i) {  // right side of the bomb
                    if (column + i <= 10) {
                        if ((column + i) % 2 == 1 && row % 2 == 1)    // unbreakable block
                            break;
                        if (blocks[row][column+i] == 1) {
                            blocks[row][column+i] = 0;
                            list_append(&destroyed_blocks, (void *)(row * 11 + (column+i)));
                            break;
                        }
                    }
                }
                for (int i = 1; i <= 2; ++i) {  // left side of the bomb
                    if (column - i >= 0) {
                        if ((column - i) % 2 == 1 && row % 2 == 1)    // unbreakable block
                            break;
                        if (blocks[row][column-i] == 1) {
                            blocks[row][column-i] = 0;
                            list_append(&destroyed_blocks, (void *)(row * 11 + (column-i)));
                            break;
                        }
                    }
                }
                pthread_mutex_unlock(&blocks_mutex);
            }

            // create kill zone in range of the bomb and then kill the players who are in it
            if (tick >= content->start_of_explosion) {
                pthread_mutex_lock(&blocks_mutex);
                // translate tile number to row&column pair
                int row = content->tile / 11;
                int column = content->tile % 11;

                int lava[9]; // blocks full of lava which are the "kill zone"
                for (int i = 0; i < 9; ++i) {
                    lava[i] = -1;           // -1 is invalid tile number so it means it's empty
                }
                int lava_counter = 0;
                lava[lava_counter++] = content->tile;
                // find blocks which are "kill zone" nn each side of the bomb
                for (int i = 1; i <= 2; ++i) {  // below
                    if (row + i <= 10) {
                        if ((column % 2 == 1 && (row + i) % 2 == 1) || blocks[row+i][column] == 1)    // walls
                            break;
                        lava[lava_counter++] = (row + i) * 11 + column;
                    }
                }
                for (int i = 1; i <= 2; ++i) {  // above
                    if (row - i >= 0) {
                        if ((column % 2 == 1 && (row - i) % 2 == 1) || blocks[row-i][column] == 1)    // walls
                            break;

                        lava[lava_counter++] = (row - i) * 11 + column;
                    }
                }
                for (int i = 1; i <= 2; ++i) {  // right side of the bomb
                    if (column + i <= 10) {
                        if (((column + i) % 2 == 1 && row % 2 == 1) || blocks[row][column+i] == 1)    // walls
                            break;
                        lava[lava_counter++] = row * 11 + (column + i);
                    }
                }
                for (int i = 1; i <= 2; ++i) {  // left side of the bomb
                    if (column - i >= 0) {
                        if (((column - i) % 2 == 1 && row % 2 == 1) || blocks[row][column-i] == 1)    // walls
                            break;
                        lava[lava_counter++] = row * 11 + (column - i);
                    }
                }
                // Kill zone is created now.
                // Now check if the player is inside this "kill zone".
                pthread_mutex_unlock(&blocks_mutex);
                pthread_mutex_lock(&players_mutex);
                list_t *player = players_root.next;
                while (player != NULL) {
                    player_t *player_content = player->content;

                    int x = player_content->x, y = player_content->y;
                    // count at which tiles are all the player's corners
                    int corners[4];
                    // left top
                    corners[0] = ((y - 41 - 40 / 2) / 58) * 11
                                 + ((x - 321 - 40 / 2) / 58);
                    // right top
                    corners[1] = ((y - 41 - 40 / 2) / 58) * 11
                                 + ((x - 321 + 40 / 2 - 1) / 58);
                    // right bottom
                    corners[2] = ((y - 41 + 40 / 2 - 1) / 58) * 11
                                 + ((x - 321 + 40 / 2 - 1) / 58);
                    // left bottom
                    corners[3] = ((y - 41 + 40 / 2 - 1) / 58) * 11
                                 + ((x - 321 - 40 / 2) / 58);


                    // compare tiles where the player is with the "kill zone"
                    for (int i = 0; i < 4; ++i) {
                        for (int j = 0; j < 9; ++j) {
                            if (lava[j] == -1)
                                break;

                            if (corners[i] == lava[j]) {
                                player_content->alive = 0;      // if player is in kill zone, then he is killed
                            }
                        }
                    }

                    player = player->next;
                }
                pthread_mutex_unlock(&players_mutex);
            }

            // Delete the bomb when the explostion ends.
            if (tick >= content->end_of_life) {
                list_t *backup = temp->next;
                list_remove(&bombs_root, temp->content);
                pthread_mutex_unlock(&players_mutex);
                temp = backup;
                if (backup == NULL)
                    break;
            }
        }
        pthread_mutex_unlock(&bombs_mutex);


        // Next place in the message is for the list of players with their current positions and state
        temp = &players_root;
        pthread_mutex_lock(&players_mutex);
        sprintf(buffer, "%s%d %d\n", buffer, players_msg, list_length(&players_root));
        while (temp->next != NULL) {
            temp = temp->next;
            player_t *content = temp->content;
            sprintf(buffer, "%s%s %d %d %d\n", buffer, content->name, content->x, content->y, content->alive);
        }
        pthread_mutex_unlock(&players_mutex);

        // Next are the blocks which were destroyed in this tick
        sprintf(buffer, "%s%d %d\n", buffer, destroyed_blocks_msg, list_length(&destroyed_blocks));
        temp = &destroyed_blocks;
        while (temp->next != NULL) {
            temp = temp->next;
            sprintf(buffer, "%s%d ", buffer, (int)temp->content);
        }

        // clear the list of destroyed blocks for the next tick
        list_free(&destroyed_blocks);

        // Send created message to all the connected players
        temp = &sockets_root;
        pthread_mutex_lock(&sockets_mutex);
        while (temp->next != NULL) {
            temp = temp->next;
            write((int) temp->content, buffer, strlen(buffer));
        }
        pthread_mutex_unlock(&sockets_mutex);

        // wait for the tick's end
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