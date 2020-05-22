#include <sys/socket.h>

#include <sys/un.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <src/sender/broadcaster.h>
#include <signal.h>

#include "game/player.h"
#include "receiver/receiver.h"

void exitHandler(int dummy) {
    pthread_mutex_lock(&players_mutex);
    list_free(&players_root);
    pthread_mutex_lock(&sockets_mutex);
    sockets_free(&sockets_root);
    exit(0);
}

/**
 * Main only command argument is the number max player for session
 */
#pragma ide diagnostic ignored "EndlessLoop"
int main(int argc, char *argv[]) {
    // connect terminate signals with function so on ctrl+c program can free the memory
    signal(SIGTERM, exitHandler);
    signal(SIGINT, exitHandler);


    int64_t listenfd, connfd;
    //struct sockaddr_un serv_addr;
    pthread_t thread_id;
    players_root.next = NULL;
    pthread_mutex_init(&players_mutex, NULL);

    sockets_root.next = NULL;
    pthread_mutex_init(&sockets_mutex, NULL);

    bombs_root.next = NULL;
    pthread_mutex_init(&bombs_mutex, NULL);

    // open broadcaster
    pthread_create(&thread_id, NULL, broadcast, (void *)atoi(argv[1]));


    struct sockaddr_in serv_addr;
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
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
        receiver_args_t *temp = (receiver_args_t *)malloc(sizeof(receiver_args_t));
        temp->sock = connfd;
        temp->max_players = atoi(argv[1]);
        pthread_create(&thread_id, NULL, connection_handler , (void *)temp);
    }
}
