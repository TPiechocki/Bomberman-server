//
// Created by Tomasz Piechocki on 07/05/2020.
//

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "list.h"

list_t *list_append(list_t *root, void *content) {
    list_t *temp = root;
    // iterate to the end of the list
    while (temp->next != NULL)
        temp = temp->next;

    // create new element and append to list
    list_t *new_elem = (list_t *)malloc(sizeof(list_t));
    new_elem->content = content;
    new_elem->next = NULL;

    temp->next = new_elem;
    return root;
}

int list_remove(list_t *root, void *content) {
    list_t *temp = root;
    while (temp->next != NULL) {
        if (temp->next->content == content) {
            list_t *toDelete = temp->next;  // don't lose the pointer to the element to delete
            temp->next = temp->next->next;  // remove from list
            free(toDelete);   // remove from memory
            return 0;
        }
        temp = temp->next;  // next element
    }
    return 1;
}

void list_display(list_t *root) {
    list_t *temp = root;
    // iterate to the end of the list
    int counter = 1;
    while (temp->next != NULL) {
        temp = temp->next;
        printf("%d %ld\n", counter++, (int64_t)temp->content);
    }
}

void list_free(list_t *root) {
    list_t *temp = root->next, *next;
    // iterate to the end of the list
    while (temp != NULL) {
        next = temp->next;
        free(temp);
        temp = next;
    }
    root->next = NULL;
}

void sockets_free(list_t *root) {
    list_t *temp = root->next, *next;
    // iterate to the end of the list
    while (temp != NULL) {
        next = temp->next;
        printf("%ld\n", (int64_t)temp->content);
        close((int64_t)temp->content);
        free(temp);
        temp = next;
    }
}

int list_length(list_t *root) {
    list_t *temp = root;
    int counter = 0;
    while (temp->next != NULL) {
        temp = temp->next;
        counter++;
    }
    return counter;
}
