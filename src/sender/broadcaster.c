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

    int msec = 0, trigger = 10; /* 10ms */
    clock_t before, delta;

    list_t destroyed_blocks;
    destroyed_blocks.next = NULL;

    pthread_mutex_lock(&players_mutex);
    while (list_length(&players_root) < number) {
        pthread_mutex_unlock(&players_mutex);
        usleep(1000 * 100);
        // printf("%d\n" ,list_length(&players_root));
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

    pthread_mutex_lock(&blocks_mutex);
    sprintf(buffer, "%s%d\n", buffer, walls_msg);
    for (int i = 0; i < 11; ++i) {
        for (int j = 0; j < 11; ++j) {
            sprintf(buffer, "%s%d ", buffer, blocks[i][j]);
        }
    }
    pthread_mutex_unlock(&blocks_mutex);


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
        temp = &bombs_root;
        pthread_mutex_lock(&bombs_mutex);
        pthread_mutex_lock(&broadcaster_mutex);
        sprintf(buffer, "%d %d %d\n", bombs_msg, list_length(&bombs_root), tick++);
        // printf("%s", buffer);
        CURRENT_TICK = tick;
        pthread_mutex_unlock(&broadcaster_mutex);
        while (temp->next != NULL) {
            temp = temp->next;
            bomb_t *content = temp->content;
            sprintf(buffer, "%s%s %d %d %d\n", buffer, content->name, content->tile,
                    content->start_of_explostion, content->end_of_life);

            if (tick == content->start_of_explostion) {     // destroy destroyable walls
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

            if (tick >= content->start_of_explostion) {
                pthread_mutex_lock(&blocks_mutex);
                int row = content->tile / 11;
                int column = content->tile % 11;

                int lava[9]; // blocks full of lava
                for (int i = 0; i < 9; ++i) {
                    lava[i] = -1;
                }
                int lava_counter = 0;
                lava[lava_counter++] = content->tile;
                // blocks which are "kill zone"
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
                pthread_mutex_unlock(&blocks_mutex);
                pthread_mutex_lock(&players_mutex);
                list_t *player = players_root.next;
                while (player != NULL) {
                    player_t *player_content = player->content;

                    int x = player_content->x, y = player_content->y;
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


                    for (int i = 0; i < 4; ++i) {
                        for (int j = 0; j < 9; ++j) {
                            if (lava[j] == -1)
                                break;

                            if (corners[i] == lava[j]) {
                                player_content->alive = 0;
                            }
                        }
                    }

                    player = player->next;
                }
                pthread_mutex_unlock(&players_mutex);
            }

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

        temp = &players_root;

        pthread_mutex_lock(&players_mutex);
        sprintf(buffer, "%s%d %d\n", buffer, players_msg, list_length(&players_root));
        while (temp->next != NULL) {
            temp = temp->next;
            player_t *content = temp->content;
            sprintf(buffer, "%s%s %d %d %d\n", buffer, content->name, content->x, content->y, content->alive);
        }
        pthread_mutex_unlock(&players_mutex);


        sprintf(buffer, "%s%d %d\n", buffer, destroyed_blocks_msg, list_length(&destroyed_blocks));
        temp = &destroyed_blocks;
        while (temp->next != NULL) {
            temp = temp->next;
            sprintf(buffer, "%s%d ", buffer, (int)temp->content);
        }
        /*
        pthread_mutex_lock(&blocks_mutex);
        sprintf(buffer, "%s%d\n", buffer, walls_msg);
        for (int i = 0; i < 11; ++i) {
            for (int j = 0; j < 11; ++j) {
                sprintf(buffer, "%s%d ", buffer, blocks[i][j]);
            }
        }
        pthread_mutex_unlock(&blocks_mutex);*/

        list_free(&destroyed_blocks);


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