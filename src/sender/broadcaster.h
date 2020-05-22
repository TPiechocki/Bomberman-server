//
// Created by Tomasz Piechocki on 18/05/2020.
//

#ifndef SERWER_BROADCASTER_H
#define SERWER_BROADCASTER_H

#define TICK 15

/**
 * Function which inform all clients about the state of the game on each tick
 */
void *broadcast(void *number_players);

#endif //SERWER_BROADCASTER_H
