//
// Created by Tomasz Piechocki on 07/05/2020.
//

#include "../structures/list.h"

#ifndef SERWER_PLAYER_H
#define SERWER_PLAYER_H

typedef struct {
    char name[64];
    int x;
    int y;
    int counter;
    int connected; // 1 if connected, 0 if disconnected
} player_t;

/**
 * Change connected flag to disconnect for a player with a nick, so player can possibly reconnect.
 * @param root - root of the players list
 * @param nick - nick of the player
 */
void player_disconnect(list_t* root, char *nick);

/**
 * Display players from the list one by one - debug info
 * @param root - root of the list
 */
void players_list_display(list_t *root);

#endif //SERWER_PLAYER_H