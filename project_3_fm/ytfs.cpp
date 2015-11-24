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
#include <cstdio>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <cstdlib>

#include <iostream>
#include <string>
#include <list>
#include <unordered_map>

#include "ytfs_list.h"

using namespace std;

static const string hello_str = "Hello World!\n";
static const string hello_path = "/hello.mp3";

static string CATEGORY_DIRS[NUM_CATEGORY] = {"/Album", "/Artist", "/Genre", "/Year"};
static folder_list_t *album_folder_list = NULL;
static folder_list_t *artist_folder_list = NULL;
static folder_list_t *genre_folder_list = NULL;
static folder_list_t *year_folder_list = NULL;
static unordered_map<string, list<string>> album_mp3_map;
static unordered_map<string, list<string>> arist_mp3_map;
static unordered_map<string, list<string>> genre_mp3_map;
static unordered_map<string, list<string>> year_mp3_map;

static struct fuse_operations ytfs_oper;

void init_db(void);
bool is_directory(const string &path);
bool is_root_directory(const string &path);
bool is_category_directory(const string &path);
bool is_sub_category_directory(const string &path); 
bool is_file(const string &path);
int get_sub_directory_strlen(const string &path);

bool is_directory(const string &path)
{
    return is_root_directory(path) 
        || is_category_directory(path)
        || is_sub_category_directory(path);
}

bool is_root_directory(const string &path)
{
    return path.compare("/") == 0;
}

bool is_category_directory(const string &path) 
{
    int i;

    for (i = 0; i < NUM_CATEGORY; i++) {
        if (path.compare(CATEGORY_DIRS[i]) == 0) {
            return true;
        }
    }

    return false;
}

bool is_sub_category_directory(const string &path) 
{
    bool begins_with_category_directory = false;
    bool in_sub_category_list = true; // NEED TO FIX
    bool ends_with_mp3 = false;
    const string mp3 = ".mp3";

    for (int i = 0; i < NUM_CATEGORY; i++) {
        const string cat_dir = CATEGORY_DIRS[i];
        if (path.substr(0, cat_dir.size()) == cat_dir) {
            begins_with_category_directory = true;
            break;
        }
    }

    if (path.length() >= 4) {
        ends_with_mp3 = path.compare(path.length() - mp3.length(), mp3.length(), mp3) == 0;
    }

    return in_sub_category_list && begins_with_category_directory
        && !ends_with_mp3;
}

bool is_file(const string &path)
{
    bool begins_with_category_directory = false;
    bool ends_with_mp3 = false;
    const string mp3 = ".mp3";

    for (int i = 0; i < NUM_CATEGORY; i++) {
        const string cat_dir = CATEGORY_DIRS[i];
        if (path.substr(0, cat_dir.size()) == cat_dir) {
            begins_with_category_directory = true;
            break;
        }
    }

    if (path.length() >= 4) {
        ends_with_mp3 = path.compare(path.length() - mp3.length(), mp3.length(), mp3) == 0;
    }

    // Treat paths like /sample.mp3 and /Album/sample.mp3 and /Album/<path>/sample.3
    // valid file paths 
    return (begins_with_category_directory && ends_with_mp3) || (ends_with_mp3);
}

int get_sub_directory_strlen(const string &path) {
    folder_t *curr = NULL;
    const string genre = "/Genre";    

    if (path.substr(0, genre.size()) == genre) {
        curr = genre_folder_list->root;
        while (curr != NULL) {
            if (curr->path_name->compare(path) == 0) {
                return curr->path_name->size();
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
        stbuf->st_size = strlen(hello_str.c_str());
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
            filler(buf, CATEGORY_DIRS[i].c_str() + 1, NULL, 0);
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
            filler(buf, curr->name->c_str(), NULL, 0); // DO NOT ADD ONE to curr->name
            curr = curr->next;
        }
    }

    // Fill each sub-category folder with files
    else if (is_sub_category_directory(path)) {
        filler(buf, hello_path.c_str() + 1, NULL, 0);
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
	unsigned int len;
	(void) fi;

	if (!is_file(path))
		return -ENOENT;

	len = hello_str.size();
	if (offset < len) {
		if (offset + size > len)
			size = len - offset;
		memcpy(buf, hello_str.c_str() + offset, size);
	} else {
		size = 0;
    }

	return size;
}

int category_callback(void *data, int argc, char **argv, char **col_names) {
    for (int i = 0; i < argc; i++) {
        // printf("%s = %s\n", col_names[i], argv[i] ? argv[i] : "NULL");
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
    return 0;
}

int mp3_callback(void *data, int argc, char **argv, char **col_names) {
    string file;
    unordered_map<string, list<string>>::const_iterator mp3_it;
    list<string> mp3_list;
    string key;
    for (int i = 0; i < argc; i++) {
        if (strcmp(col_names[i], "file") == 0) {
            file = string(argv[i]);
        }
        if (strcmp(col_names[i], "album") == 0) {
            key = string("/Album/") + string(argv[i]) + string("/");
            cout << "key = " << key << endl;
            mp3_it = album_mp3_map.find(key);
            if (mp3_it != album_mp3_map.end()) {
                mp3_list = mp3_it->second;
                cout << "Found " << endl;
            }
            mp3_list.push_back(file);
            album_mp3_map[key] = mp3_list;
        }
        if (strcmp(col_names[i], "artist") == 0) {
        }
        if (strcmp(col_names[i], "genre") == 0) {
        }
        if (strcmp(col_names[i], "year") == 0) {
        }
    }
    return 0;
}

void init_db(void) 
{
    sqlite3 *db;
    char *err_msg = 0;
    int rc;
    string query;
    const char* data = "Callback function called";

    rc = sqlite3_open(DB_NAME, &db);
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        exit(1);
    }

    query = string("SELECT DISTINCT album from mp3 ORDER BY album ASC;");
    rc = sqlite3_exec(db, query.c_str(), category_callback, (void*)data, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
    } else {
        fprintf(stdout, "Operation done successfully\n");
    }

    query = string("SELECT DISTINCT artist from mp3 ORDER BY artist ASC;");
    rc = sqlite3_exec(db, query.c_str(), category_callback, (void*)data, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
    } else {
        fprintf(stdout, "Operation done successfully\n");
    }

    query = string("SELECT DISTINCT genre from mp3 ORDER BY genre ASC;");
    rc = sqlite3_exec(db, query.c_str(), category_callback, (void*)data, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
    } else {
        fprintf(stdout, "Operation done successfully\n");
    }

    query = string("SELECT DISTINCT year from mp3 ORDER BY year ASC;");
    rc = sqlite3_exec(db, query.c_str(), category_callback, (void*)data, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
    } else {
        fprintf(stdout, "Operation done successfully\n");
    }

    query = string("SELECT file, album from mp3 ORDER BY album, file ASC;");
    rc = sqlite3_exec(db, query.c_str(), mp3_callback, (void*)data, &err_msg);
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
    ytfs_oper.getattr = ytfs_getattr;	
	ytfs_oper.open = ytfs_open;
	ytfs_oper.read = ytfs_read;
    ytfs_oper.readdir = ytfs_readdir;

    album_folder_list = (folder_list_t *) malloc(sizeof(folder_list_t));  
    artist_folder_list = (folder_list_t *) malloc(sizeof(folder_list_t));  
    genre_folder_list = (folder_list_t *) malloc(sizeof(folder_list_t));    
    year_folder_list = (folder_list_t *) malloc(sizeof(folder_list_t));  

    init_db();

    unordered_map<string, list<string>>::const_iterator mp3_it;
    mp3_it = album_mp3_map.find(string("/Album/Unknown/"));
    list<string> mp3_list = mp3_it->second;
    cout << "UNKNOWN COUNT = " << mp3_list.size() << endl;

    cout << endl;
    cout << endl;
    std::list<string>::const_iterator iterator;
    for (iterator = mp3_list.begin(); iterator != mp3_list.end(); ++iterator) {
        std::cout << *iterator << std::endl;
    }

    return fuse_main(argc, argv, &ytfs_oper, NULL);
}

