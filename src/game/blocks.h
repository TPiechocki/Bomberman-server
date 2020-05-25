//
// Created by Tomasz Piechocki on 23/05/2020.
//

#include <pthread.h>

#ifndef SERWER_BLOCKS_H
#define SERWER_BLOCKS_H

// state of the destroyable block and mutex for this; 0 means no block in the tile, 1 means the block is present
int blocks[11][11];
pthread_mutex_t blocks_mutex;


/**
 * Initialization of blocks which can be destroyed.
 */
void initBlocks();

#endif //SERWER_BLOCKS_H