//
// Created by Tomasz Piechocki on 23/05/2020.
//

#include "blocks.h"

void initBlocks() {
    pthread_mutex_lock(&blocks_mutex);
    for (int i = 0; i < 11; ++i) {
        for (int j = 0; j < 11; ++j) {
            if ((i == 0 && (j == 0 || j == 1)) || (i == 1 && j == 0) || (i == 9 && j == 10)
                    || (i == 10 && (j == 10 || j == 9)) || (i == 0 && (j == 10 || j == 9))
                    || (i == 1 && j == 10 ) || ((i == 9 || i == 10 ) && j == 0) || (i == 10 && j == 1))
                blocks[i][j] = 0;
            else
                blocks[i][j] = 1;
        }
    }
    pthread_mutex_unlock(&blocks_mutex);
}