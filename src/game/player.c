//
// Created by Tomasz Piechocki on 07/05/2020.
//

#include <stdlib.h>
#include <stdio.h>

#include "player.h"

void player_disconnect(list_t* root, char *nick) {
    list_t *temp = root;
    while (temp->next != NULL) {
        player_t *content = temp->next->content;
        if (content->name == nick) {
            content->connected = 0;
            printf("Player %s temporarily disconnected.", nick);
            return;
        }
        temp = temp->next;  // next element
    }
}

void players_list_display(list_t *root) {
    list_t *temp = root;
    // iterate to the end of the list
    int counter = 1;
    while (temp->next != NULL) {
        temp = temp->next;
        player_t *content = temp->content;
        printf("%d %s %d\n", counter++, content->name, content->connected);
    }
}