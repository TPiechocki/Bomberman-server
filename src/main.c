#include <sys/socket.h>

#include <sys/un.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <src/sender/broadcaster.h>

#include "game/player.h"
#include "receiver/receiver.h"

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

    // open broadcaster
    pthread_create(&thread_id, NULL, broadcast, 0);


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
