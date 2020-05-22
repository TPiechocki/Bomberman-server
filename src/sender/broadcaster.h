//
// Created by Tomasz Piechocki on 18/05/2020.
//

#include <pthread.h>

#ifndef SERWER_BROADCASTER_H
#define SERWER_BROADCASTER_H

#define TICK 15

int CURRENT_TICK;
pthread_mutex_t broadcaster_mutex;

/**
 * Function which inform all clients about the state of the game on each tick
 */
void *broadcast(void *number_players);

#endif //SERWER_BROADCASTER_H
