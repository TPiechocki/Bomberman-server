//
// Created by Tomasz Piechocki on 22/05/2020.
//

#ifndef SERWER_BOMB_H
#define SERWER_BOMB_H

#define BOMB_TICKS 100
#define LAVA_TICKS 50

typedef struct {
    char name[64];
    int tile;
    int counter;
    int start_of_explostion;
    int end_of_life;
} bomb_t;

#endif //SERWER_BOMB_H
