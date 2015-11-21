#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>

typedef struct symlink_node {
    char *path;
    char *in_folder;
    struct symlink_node *next;
} symlink_node_t;

typedef struct symlink_list {
    symlink_node_t *root;
} symlink_list_t;

void print_usage(void);
void read_files_in_root_dir(char *root_dir);
symlink_node_t *init_node(char *path, char *in_folder);

int main(int argc, char **argv)
{
    if (argc != 3) {
        print_usage();
        return EXIT_FAILURE;
    }

    symlink_list_t list = (symlink_list_t *) malloc(sizeof(symlink_list_t));
    if (list == NULL) {
        printf("Error malloc().\n");
        return EXIT_FAILURE;
    }
    
    return 0;
}

void print_usage(void)
{
    printf("Usage: <rootDir> <mountDir>\n");
}

symlink_node_t *init_node(char *path, char *in_folder)
{
    symlink_node_t *node = (symlink_node_t *) malloc(sizeof(symlink_node_t));
    if (node == NULL) {
        printf("Error malloc().\n");
        return EXIT_FAILURE;
    }
    node->path = path;
    node->in_folder = folder;
    return node;
}

void read_files_in_root_dir(char *root_dir)
{
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(root_dir)) != NULL) {
      while ((ent = readdir (dir)) != NULL) {
        printf("%s\n", ent->d_name);
      }
      closedir(dir);
    } else {
      perror("Error opening root directory.\n");
      return EXIT_FAILURE;
    }
}
