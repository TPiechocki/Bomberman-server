#include <sys/socket.h>

#include <sys/un.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include "game/player.h"

// player list root
list_t players_root;
pthread_mutex_t players_mutex;

// current sockets list root
list_t sockets_root;
pthread_mutex_t sockets_mutex;

void *
connection_handler(void *socket_desc) {

    /* Get the socket descriptor */
    int sock = * (int64_t *)socket_desc;
    ssize_t read_size;
    char *message , nick[65];

    /* Send some messages to the client */
    message = "Greetings! Gimme your name!\n";
    // write(sock , message , strlen(message));

    read_size = recv(sock , nick , 2000 , 0);
    nick[read_size] = '\0';
    printf("Client message: %s\n", nick);

    // TODO test reconnection
    pthread_mutex_lock(&players_mutex);
    int state = players_check_existence(&players_root, nick);

    if (state == 0) {
        player_t *new_player = (player_t *) malloc(sizeof(player_t));
        strcpy(new_player->name, nick);
        new_player->connected = 1;
        list_append(&players_root, (void *) new_player);
        players_list_display(&players_root);
        pthread_mutex_unlock(&players_mutex);
    } else if (state == 2) {
        pthread_mutex_unlock(&players_mutex);
        close(sock);
        pthread_exit(NULL);
    }

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
    list_remove(&sockets_root, (void *)sock);
    list_display(&sockets_root);
    pthread_mutex_unlock(&sockets_mutex);

    close(sock);
    pthread_exit(NULL);
}

#pragma ide diagnostic ignored "EndlessLoop"
int
main(int argc, char *argv[]) {
    int64_t listenfd, connfd;
    //struct sockaddr_un serv_addr;
    pthread_t thread_id;
    players_root.next = NULL;
    pthread_mutex_init(&players_mutex, NULL);

    sockets_root.next = NULL;
    pthread_mutex_init(&sockets_mutex, NULL);

    struct sockaddr_in serv_addr;
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons( 5000 );

    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

    listen(listenfd, 1000);

    while (1) {
        connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);

        // Add socket to the list
        pthread_mutex_lock(&sockets_mutex);
        list_append(&sockets_root, (void *)connfd);
        list_display(&sockets_root);
        pthread_mutex_unlock(&sockets_mutex);

        fprintf(stderr, "Connection accepted\n");
        pthread_create(&thread_id, NULL, connection_handler , (void *)&connfd);
    }
}
