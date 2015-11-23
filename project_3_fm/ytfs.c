/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall hello.c `pkg-config fuse --cflags --libs` -o hello
*/

#define FUSE_USE_VERSION 26
#define NUM_CATEGORY 4
#define DB_NAME "ytfs.db"
#define TABLE_NAME "mp3"

#include <fuse.h>
#include <sqlite3.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>

#include "ytfs_list.h"

static const char *hello_str = "Hello World!\n";
static const char *hello_path = "/hello.mp3";

static const char *CATEGORY_DIRS[NUM_CATEGORY] = {"/Album", "/Artist", "/Genre", "/Year"};
static folder_list_t *album_folder_list = NULL;
static folder_list_t *artist_folder_list = NULL;
static folder_list_t *genre_folder_list = NULL;
static folder_list_t *year_folder_list = NULL;

void init_db(void);
bool is_directory(const char *path);
bool is_root_directory(const char *path);
bool is_category_directory(const char *path);
bool is_sub_category_directory(const char *path); 
bool is_file(const char *path);
size_t get_sub_directory_strlen(const char *path);

bool is_directory(const char *path)
{
    return is_root_directory(path) 
        || is_category_directory(path)
        || is_sub_category_directory(path);
}

bool is_root_directory(const char *path)
{
    return strcmp(path, "/") == 0;
}

bool is_category_directory(const char *path) 
{
    int i;

    for (i = 0; i < NUM_CATEGORY; i++) {
        if (strcmp(path, CATEGORY_DIRS[i]) == 0) {
            return true;
        }
    }

    return false;
}

bool is_sub_category_directory(const char *path) 
{
    bool begins_with_category_directory = false;
    bool in_sub_category_list = true; // NEED TO FIX
    bool ends_with_mp3 = false;
    int i;

    for (i = 0; i < NUM_CATEGORY; i++) {
        const char *cat_dir = CATEGORY_DIRS[i];
        // If the category contains the path but isn't the
        // same length as the category, then true
        if ((strncmp(cat_dir, path, strlen(cat_dir)) == 0) &&
            (strlen(path) != strlen(cat_dir))) {
            begins_with_category_directory = true;
            break;
        }
    }

    size_t size = strlen(path);
    if (size >= 4 &&
        path[size - 4] == '.' &&
        path[size - 3] == 'm' &&
        path[size - 2] == 'p' &&
        path[size - 1] == '3') {
        ends_with_mp3 = true;
    } 

    return in_sub_category_list && begins_with_category_directory
        && !ends_with_mp3;
}

bool is_file(const char *path)
{
    bool begins_with_category_directory = false;
    bool ends_with_mp3 = false;
    int i;

    for (i = 0; i < NUM_CATEGORY; i++) {
        const char *cat_dir = CATEGORY_DIRS[i];
        if (strncmp(cat_dir, path, strlen(cat_dir)) == 0) {
            begins_with_category_directory = true;
            break;
        }
    }

    size_t size = strlen(path);
    if (size >= 4 &&
        path[size - 4] == '.' &&
        path[size - 3] == 'm' &&
        path[size - 2] == 'p' &&
        path[size - 1] == '3') {
        ends_with_mp3 = true;
    } 

    // Treat paths like /sample.mp3 and /Album/sample.mp3 and /Album/<path>/sample.3
    // valid file paths 
    return (begins_with_category_directory && ends_with_mp3) || (ends_with_mp3);
}

size_t get_sub_directory_strlen(const char *path) {
    folder_t *curr = NULL;
    
    if (strncmp("/Genre", path, strlen("/Genre")) == 0) {
        curr = genre_folder_list->root;
        while (curr != NULL) {
            if (strcmp(curr->path_name, path) == 0) {
                return strlen(curr->path_name);
            }
            curr = curr->next;
        }
    }

    return -1;
}

static int ytfs_getattr(const char *path, struct stat *stbuf)
{
	int res = 0;

	memset(stbuf, 0, sizeof(struct stat));
	if (is_root_directory(path)) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 4;
	} else if (is_category_directory(path)) { 
        stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 3;
    } else if (is_sub_category_directory(path)) {
        stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
    } else if (is_file(path)) {
        stbuf->st_mode = S_IFREG | 0666;
        stbuf->st_nlink = 1;
        stbuf->st_size = strlen(hello_str);
    } else {
		res = -ENOENT;
    }

	return res;
}

static int ytfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi)
{
	(void) offset;
	(void) fi;
    int i;

    // Since we're reading directories, reject all non-directories
	if (!is_directory(path))
		return -ENOENT;

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

    // Populate the root directory with the core category folders
    if (is_root_directory(path)) {
        for (i = 0; i < NUM_CATEGORY; i++) {
            filler(buf, CATEGORY_DIRS[i] + 1, NULL, 0);
        }
    }

    // Fill each category folder with specific folder names
    else if (is_category_directory(path)) {
        folder_t *curr = NULL;
        if (strcmp(path, "/Album") == 0) {
            curr = album_folder_list->root;
        } else if (strcmp(path, "/Artist") == 0) {
            curr = artist_folder_list->root;
        } else if (strcmp(path, "/Genre") == 0) {
            curr = genre_folder_list->root;
        } else if (strcmp(path, "/Year") == 0) {
            curr = year_folder_list->root;
        }
        while (curr != NULL) { 
            filler(buf, curr->name, NULL, 0); // DO NOT ADD ONE to curr->name
            curr = curr->next;
        }
    }

    // Fill each sub-category folder with files
    else if (is_sub_category_directory(path)) {
        filler(buf, hello_path + 1, NULL, 0);
    }

	return 0;
}

static int ytfs_open(const char *path, struct fuse_file_info *fi)
{
	if (!is_file(path))
		return -ENOENT;

	if ((fi->flags & 3) != O_RDONLY)
		return -EACCES;

	return 0;
}

static int ytfs_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	size_t len;
	(void) fi;

	if (!is_file(path))
		return -ENOENT;

	len = strlen(hello_str);
	if (offset < len) {
		if (offset + size > len)
			size = len - offset;
		memcpy(buf, hello_str + offset, size);
	} else {
		size = 0;
    }

	return size;
}

static struct fuse_operations ytfs_oper = {
	.getattr	= ytfs_getattr,
	.readdir	= ytfs_readdir,
	.open		= ytfs_open,
	.read		= ytfs_read,
};

int callback(void *data, int argc, char **argv, char **col_names) {
    int i;
    fprintf(stdout, "%s: ", (const char*) data);
    for (i = 0; i < argc; i++) {
        printf("%s = %s\n", col_names[i], argv[i] ? argv[i] : "NULL");
        if (strcmp(col_names[i], "album") == 0) {
            insert_node(album_folder_list, argv[i], "/Album/");
        }
        if (strcmp(col_names[i], "artist") == 0) {
            insert_node(artist_folder_list, argv[i], "/Artist/");
        }
        if (strcmp(col_names[i], "genre") == 0) {
            insert_node(genre_folder_list, argv[i], "/Genre/");
        }
        if (strcmp(col_names[i], "year") == 0) {
            insert_node(year_folder_list, argv[i], "/Year/");
        }
    }
    printf("\n");
    return 0;
}

void init_db(void) 
{
    sqlite3 *db;
    char *err_msg = 0;
    int rc;
    char *query;
    const char* data = "Callback function called";

    rc = sqlite3_open(DB_NAME, &db);
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        exit(1);
    }

    query = "SELECT DISTINCT album from mp3 ORDER BY album ASC;";
    rc = sqlite3_exec(db, query, callback, (void*)data, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
    } else {
        fprintf(stdout, "Operation done successfully\n");
    }

    query = "SELECT DISTINCT artist from mp3 ORDER BY artist ASC;";
    rc = sqlite3_exec(db, query, callback, (void*)data, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
    } else {
        fprintf(stdout, "Operation done successfully\n");
    }

    query = "SELECT DISTINCT genre from mp3 ORDER BY genre ASC;";
    rc = sqlite3_exec(db, query, callback, (void*)data, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
    } else {
        fprintf(stdout, "Operation done successfully\n");
    }

    query = "SELECT DISTINCT year from mp3 ORDER BY year ASC;";
    rc = sqlite3_exec(db, query, callback, (void*)data, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
    } else {
        fprintf(stdout, "Operation done successfully\n");
    }

    sqlite3_close(db);
}

int main(int argc, char *argv[])
{
    album_folder_list = (folder_list_t *) malloc(sizeof(folder_list_t));  
    artist_folder_list = (folder_list_t *) malloc(sizeof(folder_list_t));  
    genre_folder_list = (folder_list_t *) malloc(sizeof(folder_list_t));    
    year_folder_list = (folder_list_t *) malloc(sizeof(folder_list_t));  

    init_db();
    return fuse_main(argc, argv, &ytfs_oper, NULL);
}

