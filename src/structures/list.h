//
// Created by Tomasz Piechocki on 07/05/2020.
//

#ifndef SERWER_LIST_H
#define SERWER_LIST_H

typedef struct list {
    struct list *next;
    void *content;
} list_t;

// player list root
list_t players_root;
pthread_mutex_t players_mutex;

// bombs list root
list_t bombs_root;
pthread_mutex_t bombs_mutex;

// current sockets list root
list_t sockets_root;
pthread_mutex_t sockets_mutex;

/**
 * Append element at the end of the list
 * @param root - list root
 * @param content - content of the element
 * @return - root
 */
list_t *list_append(list_t *root, void *content);

/**
 * Remove element from the list which has the same content
 * @param root - list root
 * @param content  - content of the element to remove
 * @return - 0 if removed, 1 if nothing was to delete
 */
int list_remove(list_t *root, void *content);

/**
 * Display element one by one - debug info
 * @param root - root of the list
 */
void list_display(list_t *root);

/**
 * Remove all elements from the memory
 * @param root - root of the list
 */
void list_free(list_t *root);

/**
 * Free element's memory
 * @param root - root of the list
 */
void sockets_free(list_t *root);

/**
 * @param root - root of the list
 * @return number of elements in the list
 */
int list_length(list_t *root);

#endif //SERWER_LIST_H
