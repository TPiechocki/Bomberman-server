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
}MSG;


/**
 * Function to get all individual client messages.
 * @param socket_desc - socket with the client
 */
void *connection_handler(void *socket_desc);

#endif //SERWER_RECEIVER_H