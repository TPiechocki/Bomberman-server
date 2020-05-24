//
// Created by Tomasz Piechocki on 23/05/2020.
//

#include <pthread.h>

#ifndef SERWER_BLOCKS_H
#define SERWER_BLOCKS_H

// player list root
int blocks[11][11];
pthread_mutex_t blocks_mutex;


/**
 * Initizalization of block which can be destroyed.
 */
void initBlocks();

#endif //SERWER_BLOCKS_H