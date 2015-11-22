#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>

typedef struct symlink_node {
    const char *path;
    const char *in_folder;
    struct symlink_node *next;
} symlink_node_t;

typedef struct symlink_list {
    symlink_node_t *root;
} symlink_list_t;

void print_usage(void);
void read_files_in_root_dir(char *root_dir, symlink_list_t *list);
void create_symlinks_in_mount_dir(char *mount_dir, symlink_list_t *list);
symlink_node_t *init_node(const char *path, const char *in_folder);
void insert_node(symlink_list_t *list, const char *path, const char *in_folder);

const char *CURRENT_DIR_PATH;

symlink_node_t *init_node(const char *path, const char *in_folder)
{
    symlink_node_t *node = (symlink_node_t *) malloc(sizeof(symlink_node_t));
    if (node == NULL) {
        printf("Error malloc().\n");
        exit(1);
    }
    node->path = (const char *) malloc(sizeof(char) * strlen(path));
    strcpy(node->path, path);
    node->in_folder = in_folder;
    node->next = NULL;
    return node;
}

void insert_node(symlink_list_t *list, const char *path, const char *in_folder)
{
    symlink_node_t *node = init_node(path, in_folder);
    if (list->root == NULL) {
        list->root = node;
    } else {
        node->next = list->root;
        list->root = node;
    }
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        print_usage();
        return EXIT_FAILURE;
    }

    symlink_list_t *list = (symlink_list_t *) malloc(sizeof(symlink_list_t));
    if (list == NULL) {
        printf("Error malloc().\n");
        return EXIT_FAILURE;
    }
    
    CURRENT_DIR_PATH = (const char *) get_current_dir_name();
    read_files_in_root_dir(argv[1], list);
    create_symlinks_in_mount_dir(argv[2], list);
    
    return 0;
}

void print_usage(void)
{
    printf("Usage: <rootDir> <mountDir>\n");
}

void read_files_in_root_dir(char *root_dir, symlink_list_t *list)
{
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(root_dir)) != NULL) {
      while ((ent = readdir (dir)) != NULL) {
        // Ignore these hidden "files"
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
            continue;
        }
      
        char path[strlen(ent->d_name) + strlen(CURRENT_DIR_PATH) + 2];
        strcpy(path, CURRENT_DIR_PATH);
        strcat(path, "/");
        strcat(path, root_dir);
        strcat(path, "/");
        strcat(path, ent->d_name);
        path[strlen(path)] = '\0';
        insert_node(list, path, ".");
      }
      closedir(dir);
    } else {
      perror("Error opening root directory.\n");
      return;
    }
}

void create_symlinks_in_mount_dir(char *mount_dir, symlink_list_t *list) 
{
    printf("hello\n");
    symlink_node_t *curr = list->root;
    while (curr != NULL) {
        const char *fn = basename(curr->path);
        
        char mount_path[strlen(fn) + strlen(CURRENT_DIR_PATH) + 2];
        strcpy(mount_path, CURRENT_DIR_PATH);
        strcat(mount_path, "/");
        strcat(mount_path, mount_dir);
        strcat(mount_path, "/");
        strcat(mount_path, fn);
        mount_path[strlen(mount_path)] = '\0';
        
        printf("%s %s\n", curr->path, mount_path);
        symlink(curr->path, mount_path);
        
        curr = curr->next;
    }
}

