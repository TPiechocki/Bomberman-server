//
// Created by Tomasz Piechocki on 18/05/2020.
//

#ifndef SERWER_RECEIVER_H
#define SERWER_RECEIVER_H


typedef enum Messages_ids{
    move_msg,
    bomb_msg,
    name_msg,
    start_msg,
    end_msg,
    players_msg,
    bombs_msg,
    walls_msg,
    destroyed_blocks_msg
}MSG;


typedef struct {
    int sock;
    int max_players;
} receiver_args_t;

/**
 * Function to get all individual client messages.
 * @param socket_desc - arguments for the thread
 */
void *connection_handler(void *receiver_args);

#endif //SERWER_RECEIVER_H