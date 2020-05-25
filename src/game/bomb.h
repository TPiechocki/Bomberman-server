//
// Created by Tomasz Piechocki on 22/05/2020.
//

#ifndef SERWER_BOMB_H
#define SERWER_BOMB_H

// Both these const values are in TICKS unit
#define BOMB_TICKS 100      // how long to start of the explosion since the bomb is placed
#define LAVA_TICKS 50       // length of explosion

typedef struct {
    char name[64];
    int tile;
    int counter;
    int start_of_explosion;
    int end_of_life;
} bomb_t;

#endif //SERWER_BOMB_H
