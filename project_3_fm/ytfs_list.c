#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "ytfs_list.h"

folder_t *init_node(const char *name)
{
    folder_t *node = (folder_t *) malloc(sizeof(folder_t));
    if (node == NULL) {
        printf("Error malloc().\n");
        exit(1);
    }
    node->name = (char *) malloc(sizeof(char) * strlen(name));
    strcpy(node->name, name);
    node->next = NULL;
    return node;
}

void insert_node(folder_list_t *list, const char *name)
{
    folder_t *node = init_node(name);
    if (list->root == NULL) {
        list->root = node;
    } else {
        node->next = list->root;
        list->root = node;
    }
}