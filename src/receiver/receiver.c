//
// Created by Tomasz Piechocki on 18/05/2020.
//

#include "receiver.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <sys/socket.h>
#include <pthread.h>
#include <src/structures/list.h>
#include <src/game/player.h>

void *
connection_handler(void *socket_desc) {

    /* Get the socket descriptor */
    int sock = * (int64_t *)socket_desc;
    ssize_t read_size;
    char *message , nick[65];

    player_t *this_player;

    /* Send some messages to the client */
    message = "Greetings! Gimme your name!\n";
    // write(sock , message , strlen(message));

    read_size = recv(sock , nick , 2000 , 0);
    nick[read_size] = '\0';
    printf("Client message: %s\n", nick);

    // TODO test reconnection
    pthread_mutex_lock(&players_mutex);
    int64_t state = players_check_existence(&players_root, nick);

    if (state == 0) {       // create new player
        this_player = (player_t *) malloc(sizeof(player_t));
        strcpy(this_player->name, nick);
        this_player->connected = 1;
        list_append(&players_root, (void *)this_player);
        players_list_display(&players_root);
        pthread_mutex_unlock(&players_mutex);
    } else if (state == -1) {   // player couldn't be connected
        pthread_mutex_unlock(&players_mutex);
        close(sock);
        pthread_exit(NULL);
    } else      // reconnected player
        this_player = (player_t *)state;

    char buffer[2000];
    do {
        read_size = recv(sock , nick , 2000 , 0);
        nick[read_size] = '\0';
        int x, y, length;
        int counter;
        sscanf(nick, "%d %s %d %d %d\n", &length, buffer, &x, &y, &counter);
        printf("Client move: %s\n", buffer);

        /* Send the message back to client */
        // write(sock , nick , strlen(buffer));

        /* Clear the message buffer */
        memset(buffer, 0, 2000);
    } while(read_size > 2); /* Wait for empty line */

    fprintf(stderr, "Client disconnected\n");

    // TODO decide when to remove player completely
    pthread_mutex_lock(&players_mutex);
    player_disconnect(&players_root, nick);
    players_list_display(&players_root);
    pthread_mutex_unlock(&players_mutex);

    // Remove socket when disconnected
    pthread_mutex_lock(&sockets_mutex);
    list_remove(&sockets_root, (void *)(int64_t)sock);
    list_display(&sockets_root);
    pthread_mutex_unlock(&sockets_mutex);

    close(sock);
    pthread_exit(NULL);
}
