#ifndef __YTFS_LIST__
#define __YTFS_LIST__

typedef struct folder {
    char *name;
    char *path_name;
    struct folder *next;
} folder_t;

typedef struct folder_list {
    folder_t *root;
} folder_list_t;

folder_t *init_node(const char *name, const char *parent_name);
void insert_node(folder_list_t *list, const char *name, const char *parent_name);

#endif
