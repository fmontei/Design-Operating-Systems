#include <cstring>
#include <cstdlib>
#include <cstdio>

#include <string>
#include <iostream>

#include "ytfs_list.h"

using namespace std;

folder_t *init_node(const char *name, const char *parent_name)
{
    folder_t *node = (folder_t *) malloc(sizeof(folder_t));
    if (node == NULL) {
        printf("Error malloc().\n");
        exit(1);
    }
    node->name = new string(name);
    node->path_name = new string(string(name) + string(parent_name));
    node->next = NULL;
    return node;
}

void insert_node(folder_list_t *list, const char *name, const char *parent_name)
{
    folder_t *node = init_node(name, parent_name);
    if (list->root == NULL) {
        list->root = node;
    } else {
        node->next = list->root;
        list->root = node;
    }
}
