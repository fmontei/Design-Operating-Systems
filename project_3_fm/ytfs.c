/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall hello.c `pkg-config fuse --cflags --libs` -o hello
*/

#define FUSE_USE_VERSION 26
#define NUM_CATEGORY 4

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>

static const char *hello_str = "Hello World!\n";
static const char *hello_path = "/hello.mp3";

static const char *CATEGORY_DIRS[NUM_CATEGORY] = {"/Album", "/Artist", "/Genre", "/Year"};

bool is_directory(const char *path);
bool is_root_directory(const char *path);
bool is_category_directory(const char *path); 
bool is_file(const char *path);

static int hello_getattr(const char *path, struct stat *stbuf)
{
	int res = 0;

	memset(stbuf, 0, sizeof(struct stat));
	if (is_root_directory(path)) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 3;
	} else if (is_category_directory(path)) { 
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

static int hello_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
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

    // Fill each category folder with files
    if (is_category_directory(path)) {
        filler(buf, hello_path + 1, NULL, 0);
    }

	return 0;
}

static int hello_open(const char *path, struct fuse_file_info *fi)
{
	if (!is_file(path))
		return -ENOENT;

	if ((fi->flags & 3) != O_RDONLY)
		return -EACCES;

	return 0;
}

static int hello_read(const char *path, char *buf, size_t size, off_t offset,
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

static struct fuse_operations hello_oper = {
	.getattr	= hello_getattr,
	.readdir	= hello_readdir,
	.open		= hello_open,
	.read		= hello_read,
};

int main(int argc, char *argv[])
{
	return fuse_main(argc, argv, &hello_oper, NULL);
}

bool is_directory(const char *path)
{
    return is_root_directory(path) || is_category_directory(path);
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

bool is_file(const char *path)
{
    bool begins_with_category_directory = false;
    bool ends_with_mp3 = false;
    int i;

    for (i = 0; i < NUM_CATEGORY; i++) {
        const char *cat_dir = CATEGORY_DIRS[i];
        if (strncmp(cat_dir, path, strlen(cat_dir)) == 0) {
            begins_with_category_directory = true;
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
