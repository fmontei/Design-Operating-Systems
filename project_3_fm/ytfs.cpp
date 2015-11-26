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
#include <unistd.h>

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
static unordered_map<string, list<string>> artist_mp3_map;
static unordered_map<string, list<string>> genre_mp3_map;
static unordered_map<string, list<string>> year_mp3_map;
static unordered_map<string, string> file_look_up_map; // Takes in title, returns absolute path to file

static struct fuse_operations ytfs_oper;

void init_db(void);
bool begins_with(const char *str, const char *pre);
bool is_directory(const string &path);
bool is_root_directory(const string &path);
bool is_category_directory(const string &path);
bool is_sub_category_directory(const string &path); 
bool is_file(const string &path);
int get_sub_directory_strlen(const string &path);
int mp3_callback(void *data, int argc, char **argv, char **col_names);
int category_callback(void *data, int argc, char **argv, char **col_names);
int file_look_up_callback(void *data, int argc, char **argv, char **col_names);


bool begins_with(const char *str, const char *pre) 
{
    return strncmp(pre, str, strlen(pre)) == 0;
}

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
    for (int i = 0; i < NUM_CATEGORY; i++) {
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

list<string> get_list_by_path(const char *path, unordered_map<string, list<string>> map)
{
    unordered_map<string, list<string>>::const_iterator mp3_it;
    mp3_it = map.find(string(path));
    if (mp3_it == map.end()) 
    {
        printf("Error: path not found in map. Path: %s.\n", path);
        exit(1);
    }
    return mp3_it->second;
}

string get_absolute_file_path(const char *path)
{
    string absolute_file_path;
    string file_name_in_path;
    string cpp_path = string(path);
    int index = cpp_path.find_last_of("/") + 1;
    file_name_in_path = cpp_path.substr(index);

    unordered_map<string, string>::const_iterator it;
    it = file_look_up_map.find(file_name_in_path);
    if (it == file_look_up_map.end()) {
        printf("Error: path not found in file_look_up_map. Path: %s.\n", 
            file_name_in_path.c_str());
    }
    absolute_file_path = it->second;

    return absolute_file_path;
}

size_t get_file_size(const char *path)
{
    string absolute_file_path = get_absolute_file_path(path);

    FILE *mp3_fp = fopen(absolute_file_path.c_str(), "r");
    if (mp3_fp == NULL) {
        printf("Error could not open file. Path: %s.\n", path);
        exit(1);
    }

    fseek(mp3_fp, 0L, SEEK_END);
    const size_t size = ftell(mp3_fp);
    fclose(mp3_fp);

    return size;
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
        stbuf->st_size = get_file_size(path);
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

    // Since we're reading directories, reject all non-directories
	if (!is_directory(path))
		return -ENOENT;

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

    // Populate the root directory with the core category folders
    if (is_root_directory(path)) {
        for (int i = 0; i < NUM_CATEGORY; i++) {
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
        list<string> mp3_list;
        list<string>::const_iterator it;

        if (begins_with(path, "/Album/")) {
            const string cpp_path = string(path);
            const int index = cpp_path.find("/Album/") + strlen("/Album/");
            const string album = cpp_path.substr(index);   
            mp3_list = get_list_by_path(path, album_mp3_map); 
        } else if (begins_with(path, "/Artist/")) {
            const string cpp_path = string(path);
            const int index = cpp_path.find("/Artist/") + strlen("/Artist/");
            const string artist = cpp_path.substr(index);
            mp3_list = get_list_by_path(path, artist_mp3_map);
        } else if (begins_with(path, "/Genre/")) {
            const string cpp_path = string(path);
            const int index = cpp_path.find("/Genre/") + strlen("/Genre/");
            const string genre = cpp_path.substr(index);
            mp3_list = get_list_by_path(path, genre_mp3_map);
        } else if (begins_with(path, "/Year/")) {
            const string cpp_path = string(path);
            const int index = cpp_path.find("/Year/") + strlen("/Year/");
            const string year = cpp_path.substr(index);
            mp3_list = get_list_by_path(path, year_mp3_map);    
        }

        for (it = mp3_list.begin(); it != mp3_list.end(); ++it) {
            filler(buf, (*it).c_str(), NULL, 0);
            printf("%s\n", (*it).c_str());
        }
    }

	return 0;
}

static int ytfs_open(const char *path, struct fuse_file_info *fi)
{
	if (!is_file(path))
		return -ENOENT;

	if ((fi->flags & 3) != O_RDONLY)
		return -EACCES;

    //fi->fh = get_file_handle(path);
    string absolute_file_path = get_absolute_file_path(path);
    int res = open(absolute_file_path.c_str(), fi->flags);
    if (res == -1) {
        return -ENOENT;
    }

    close(res);

    // printf("OPEN file handle = %d\n", fi->fh);

	return 0;
}

static int ytfs_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
    int fd;
    int res;
	unsigned int len;
	(void) fi;

	if (!is_file(path))
		return -ENOENT;

	/*len = hello_str.size();
	if (offset < len) {
		if (offset + size > len)
			size = len - offset;
		memcpy(buf, hello_str.c_str() + offset, size);
	} else {
		size = 0;
    }*/

    // Read in the file using the absolute file path
    // FILE *mp3 = fopen(absolute_file_path.c_str(), "r");
    //int retval = pread(fi->fh, buf, size, offset);
    //if (retval == -1)
    //{
        //retval = -ENOENT;
    //}
    string absolute_file_path = get_absolute_file_path(path);
    fd = open(absolute_file_path.c_str(), O_RDONLY);
    if (fd == -1) {
        return -ENOENT;
    }

    res = pread(fd, buf, size, offset);
    if (res == -1) {
        res = -ENOENT;
    }

    // cout << file_name_in_path << " -> " << absolute_file_path << endl;

    //printf("READ file handle = %d, buf = %s, size = %d, offset = %d\n", fi->fh, buf, (int) size, (int) offset);
    printf("PATH = %s size = %d\n", path, (int) size);

    close(fd);
	return res;
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

    query = string("SELECT DISTINCT title, album from mp3 ORDER BY album, file ASC;");
    rc = sqlite3_exec(db, query.c_str(), mp3_callback, (void*)data, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
    } else {
        fprintf(stdout, "Operation done successfully\n");
    }

    query = string("SELECT DISTINCT title, artist from mp3 ORDER BY artist, file ASC;");
    rc = sqlite3_exec(db, query.c_str(), mp3_callback, (void*)data, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
    } else {
        fprintf(stdout, "Operation done successfully\n");
    }

    query = string("SELECT DISTINCT title, genre from mp3 ORDER BY genre, file ASC;");
    rc = sqlite3_exec(db, query.c_str(), mp3_callback, (void*)data, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
    } else {
        fprintf(stdout, "Operation done successfully\n");
    }

    query = string("SELECT DISTINCT title, year from mp3 ORDER BY year, file ASC;");
    rc = sqlite3_exec(db, query.c_str(), mp3_callback, (void*)data, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
    } else {
        fprintf(stdout, "Operation done successfully\n");
    }

    // Query to populate file_look_up_map
    query = string("SELECT DISTINCT file, title from mp3;");
    rc = sqlite3_exec(db, query.c_str(), file_look_up_callback, (void*)data, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
    } else {
        fprintf(stdout, "Operation done successfully\n");
    }

    sqlite3_close(db);
}

int category_callback(void *data, int argc, char **argv, char **col_names) {
    for (int i = 0; i < argc; i++) {
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
    string key, title;
    unordered_map<string, list<string>>::const_iterator mp3_it;
    list<string> mp3_list;
    for (int i = 0; i < argc; i++) {
        if (strcmp(col_names[i], "title") == 0) {
            title = string(argv[i]);
        }
        if (strcmp(col_names[i], "album") == 0) {
            key = string("/Album/") + string(argv[i]);
            mp3_it = album_mp3_map.find(key);
            if (mp3_it != album_mp3_map.end()) {
                mp3_list = mp3_it->second;
            }
            mp3_list.push_back(title);
            album_mp3_map[key] = mp3_list;
        }
        if (strcmp(col_names[i], "artist") == 0) {
            key = string("/Artist/") + string(argv[i]);
            mp3_it = artist_mp3_map.find(key);
            if (mp3_it != artist_mp3_map.end()) {
                mp3_list = mp3_it->second;
            }
            mp3_list.push_back(title);
            artist_mp3_map[key] = mp3_list;
        }
        if (strcmp(col_names[i], "genre") == 0) {
            key = string("/Genre/") + string(argv[i]);
            mp3_it = genre_mp3_map.find(key);
            if (mp3_it != genre_mp3_map.end()) {
                mp3_list = mp3_it->second;
            }
            mp3_list.push_back(title);
            genre_mp3_map[key] = mp3_list;
        }
        if (strcmp(col_names[i], "year") == 0) {
            key = string("/Year/") + string(argv[i]);
            mp3_it = year_mp3_map.find(key);
            if (mp3_it != year_mp3_map.end()) {
                mp3_list = mp3_it->second;
            }
            mp3_list.push_back(title);
            year_mp3_map[key] = mp3_list;
        }
    }

    return 0;
}

int file_look_up_callback(void *data, int argc, char **argv, char **col_names)
{
    string file, title;
    for (int i = 0; i < argc; i++) {  
        if (strcmp(col_names[i], "file") == 0) {
            file = string(argv[i]);
        } else if (strcmp(col_names[i], "title") == 0) {
            title = string(argv[i]);
        }
    }

    file_look_up_map[title] = file;

    return 0;
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

   
    for (auto kv : file_look_up_map) {
        cout << kv.first << " " << kv.second << endl;
    }

    return fuse_main(argc, argv, &ytfs_oper, NULL);
}

