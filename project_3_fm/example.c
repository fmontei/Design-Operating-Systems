#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#if defined(__APPLE__)
#  define COMMON_DIGEST_FOR_OPENSSL
#  include <CommonCrypto/CommonDigest.h>
#  define SHA1 CC_SHA1
#else
#  include <openssl/md5.h>
#endif
#define USED_CLOCK CLOCK_PROCESS_CPUTIME_ID
#define NANOS 1000000000LL

struct timespec begin, current;
long start,elapsed,microseconds;

//MD5 Algo
char *str2md5(const char *str, int length)
{
    int n;
    MD5_CTX c;
    unsigned char digest[16];
    char *out = (char *) malloc(33);
    MD5_Init(&c);
    while (length > 0)
    {
        if (length > 512)
        {
            MD5_Update(&c, str, 512);
        } else
        {
            MD5_Update(&c, str, length);
        }
        length -= 512;
        str += 512;
    }
    MD5_Final(digest, &c);
    for (n = 0; n < 16; ++n)
    {
        snprintf(&(out[n * 2]), 16 * 2, "%02x", (unsigned int) digest[n]);
    }
    return out;
}

void log_message(const char* message)
{
    FILE* f;

    f = fopen("/home/ubuntu/cs-3210/project3/FUSE/log.txt", "a");
    fprintf(f, "%s\n", message);
    fclose(f);
}

//Start FUSE code
static int pfs_getattr(const char *path, struct stat *stbuf)
{
    clock_gettime(USED_CLOCK,&begin);
    FILE *dirlist_ptr;
    FILE *stats;
    char *line = NULL;
    char *token = NULL;
    char delim[2] = ",";

    int psize = 0; // picture size
    int res = 0;
    int found_file = 0;
    ssize_t len = 0;
    ssize_t read;
    
    stats = fopen("/tmp/rpfs/stats/getattr.txt", "a");
    memset(stbuf, 0, sizeof(struct stat));
    
    if (strcmp(path, "/") == 0)
    {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        found_file = 1;
    }
    else
    {
        dirlist_ptr = fopen("/tmp/rpfs/dir/dirlist.txt", "r");       // expecting this file to exist by default

        if (dirlist_ptr == NULL)
        {
            exit(EXIT_FAILURE);
        }

        while ((read = getline(&line, &len, dirlist_ptr)) != -1)
        {
            token = strtok(line, delim);
            psize = atoi(strtok(NULL, delim));

            if (strcmp(token, path) == 0)
            {
                stbuf->st_mode = S_IFREG | 0666;
                stbuf->st_nlink = 1;
                stbuf->st_size = psize;
                found_file = 1;
                break;
            }
        }

        fclose(dirlist_ptr);
    }

    if (!found_file)
    {
        res = -ENOENT;
    }

    clock_gettime(USED_CLOCK,&current);
    start = begin.tv_sec*NANOS + begin.tv_nsec;
    elapsed = current.tv_sec*NANOS + current.tv_nsec - start;
    microseconds = elapsed / 1000 + (elapsed % 1000 >= 500);
    fprintf(stats, "%lu\n", microseconds);
    fclose(stats);

    return res;
}

static int pfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{   
    clock_gettime(USED_CLOCK,&begin);
    FILE *dirlist_ptr;
    FILE *stats;
    char *line = NULL;
    char *token = NULL;
    char delim[2] = ",";
    ssize_t read;
    size_t len = 0;

    filler(buf, ".", NULL, 0); // filler function formats our provided strings for an ls command
    filler(buf, "..", NULL, 0);
    stats = fopen("/tmp/rpfs/stats/readdir.txt", "a");
    dirlist_ptr = fopen("/tmp/rpfs/dir/dirlist.txt", "r"); // expecting this file to exist by default

    if (dirlist_ptr == NULL)
    {
        exit(EXIT_FAILURE);
    }

    while ((read = getline(&line, &len, dirlist_ptr)) != -1) // while we are still getting line data
    {
        token = strtok(line, delim); // token = file name
        filler(buf, token + 1, NULL, 0); // hack to remove slash
    }

    clock_gettime(USED_CLOCK,&current);
    start = begin.tv_sec*NANOS + begin.tv_nsec;
    elapsed = current.tv_sec*NANOS + current.tv_nsec - start;
    microseconds = elapsed / 1000 + (elapsed % 1000 >= 500);
    fprintf(stats, "%lu\n", microseconds);
    fclose(stats);
    fclose(dirlist_ptr);

    return 0;
}

static int pfs_open(const char *path, struct fuse_file_info *fi)
{
    clock_gettime(USED_CLOCK,&begin);

    FILE *dirlist_ptr;
    FILE *py_read_ptr;
    FILE *read_ptr;
    FILE *stats;
    size_t bytes_read;
    char *line;
    size_t len;
    char *name_token;
    char *py_read_path_base = "/tmp/rpfs/pyreadpath/";
    char *read_path_base = "/tmp/rpfs/read/";
    char *py_read_path;
    char *read_path;
    char line_delim[2] = ",";
    int found_file = 0;
    int count = 0;

    dirlist_ptr = fopen("/tmp/rpfs/dir/dirlist.txt", "r"); // expecting this file to exist by default

    while ((bytes_read = getline(&line, &len, dirlist_ptr)) != -1) // while we are still getting line data
    {
        name_token = strtok(line, line_delim);  // name_token = file name

        if (strcmp(path, name_token) == 0)
        {
            py_read_path = malloc(strlen(py_read_path_base) + strlen(path));
            strcpy(py_read_path, py_read_path_base);
            strcat(py_read_path, path);
            py_read_ptr = fopen(py_read_path, "w");     // write empty file with file name we need from db to /tmp/rpfs/pyreadpath
            fclose(py_read_ptr);                   // just needed to create the file
            free(py_read_path);

            break;
        }
    }

    if (len == 0)
    {
        fclose(dirlist_ptr);
        return -ENOENT;
    }

    read_path = malloc(strlen(read_path_base) + strlen(path));
    strcpy(read_path, read_path_base);
    strcat(read_path, path);

    while (count < 3) // keep looping and checking for existence of file
    {
        sleep(1);
        read_ptr = fopen(read_path, "r");

        if (read_ptr != NULL)
        {
            found_file = 1;
            fi->fh = fileno(read_ptr);

            break;
        }

        count++;
    }

    free(read_path);

    if (!found_file)
    {
        fclose(read_ptr);
        fclose(dirlist_ptr);
        return -ENOENT;
    }

    clock_gettime(USED_CLOCK,&current);
    start = begin.tv_sec*NANOS + begin.tv_nsec;
    elapsed = current.tv_sec*NANOS + current.tv_nsec - start;
    microseconds = elapsed / 1000 + (elapsed % 1000 >= 500);

    stats = fopen("/tmp/rpfs/stats/open.txt", "a");
    fprintf(stats, "%lu\n", microseconds);
    fclose(stats);

    return 0;
}

static int pfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    clock_gettime(USED_CLOCK,&begin);
    FILE *stats;
    char *read_path_base = "/tmp/rpfs/read/";
    char *read_path;
    int res;

    read_path = malloc(strlen(read_path_base) + strlen(path));
    strcpy(read_path, read_path_base);
    strcat(read_path, path);

    res = pread(fi->fh, buf, size, offset);

    if (res == -1)
    {
        res = -ENOENT;
    }

    remove(read_path);
    free(read_path);

    clock_gettime(USED_CLOCK,&current);
    start = begin.tv_sec*NANOS + begin.tv_nsec;
    elapsed = current.tv_sec*NANOS + current.tv_nsec - start;
    microseconds = elapsed / 1000 + (elapsed % 1000 >= 500);

    stats = fopen("/tmp/rpfs/stats/read.txt", "a");
    fprintf(stats, "%lu\n", microseconds);
    fclose(stats);

    return res;
}

int pfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    clock_gettime(USED_CLOCK,&begin);
    FILE *stats;
    int res;

    (void) path;
    res = pwrite(fi->fh, buf, size, offset);

    if (res == -1)
    {
        res = -ENOENT;
    }

    clock_gettime(USED_CLOCK,&current);
    start = begin.tv_sec*NANOS + begin.tv_nsec;
    elapsed = current.tv_sec*NANOS + current.tv_nsec - start;
    microseconds = elapsed / 1000 + (elapsed % 1000 >= 500);

    stats = fopen("/tmp/rpfs/stats/write.txt", "a");
    fprintf(stats, "%lu\n", microseconds);
    fclose(stats);

    return res;
}

/**
 * Clean up filesystem
 *
 * Called on filesystem exit.
 *
 * Introduced in version 2.3
 */
void pfs_destroy(void *userdata)
{
    remove("/tmp/rpfs/dir/dirlist.txt");
    remove("/tmp/rpfs/stats/getattr.txt"); 
    remove("/tmp/rpfs/stats/open.txt");
    remove("/tmp/rpfs/stats/read.txt");
    remove("/tmp/rpfs/stats/readdir.txt");
    remove("/tmp/rpfs/stats/write.txt");
    remove("/tmp/rpfs/stats/create.txt");
    rmdir("/tmp/rpfs/pyreadpath");
    rmdir("/tmp/rpfs/read");
    rmdir("/tmp/rpfs/write");
    rmdir("/tmp/rpfs/dir");
    rmdir("/tmp/rpfs/unlink");
    rmdir("/tmp/rpfs/stats");
    rmdir("/tmp/rpfs");
    return;
}

int pfs_create(const char* path, mode_t mode, struct fuse_file_info *fi)
{
    clock_gettime(USED_CLOCK,&begin);
    FILE* create_file;
    FILE* dirlist;
    FILE* stats;
    char* file_path_base = "/tmp/rpfs/write";
    char* file_path;
    char *md5_create;
    char *line = NULL;
    char *token = NULL;
    char *md5_check; 
    char delim[2] = ",";
    ssize_t len = 0;
    ssize_t read;
    int psize = 0;

    /*
    md5_create = str2md5(path,strlen(path)); 
    dirlist = fopen("/tmp/rpfs/dir/dirlist.txt", "r");

    while ((read = getline(&line, &len, dirlist)) != -1) // while we are still getting line data
    {
        token = strtok(line, delim); // token = file name
        psize = atoi(strtok(NULL, delim));
        md5_check = strtok(NULL, delim);

        if(strcmp(md5_create,md5_check) == 0)
        {
            return -ENOENT;
        }
    }

    fclose(dirlist);
    */

    dirlist = fopen("/tmp/rpfs/dir/dirlist.txt", "a");
    fprintf(dirlist, "%s,%i\n", path, 0);
    fclose(dirlist);

    file_path = malloc(strlen(file_path_base) + strlen(path));
    strcpy(file_path, file_path_base);
    strcat(file_path, path);

    create_file = fopen(file_path, "w");
    fi->fh = fileno(create_file);
    free(file_path);

    clock_gettime(USED_CLOCK,&current);
    start = begin.tv_sec*NANOS + begin.tv_nsec;
    elapsed = current.tv_sec*NANOS + current.tv_nsec - start;
    microseconds = elapsed / 1000 + (elapsed % 1000 >= 500);

    stats = fopen("/tmp/rpfs/stats/create.txt", "a");
    fprintf(stats, "%lu\n", microseconds);
    fclose(stats);

    return 0;
}

/* Remove a file */
int pfs_unlink(const char *path)
{
    char *remove_path_base = "/tmp/rpfs/unlink";
    char *remove_path;
    FILE *remove_ptr;

    remove_path = malloc(strlen(remove_path_base) + strlen(path));
    strcpy(remove_path, remove_path_base);
    strcat(remove_path, path);

    remove_ptr = fopen(remove_path, "w");
    fclose(remove_ptr); // just need to create the file

    free(remove_path);

    return 0;
}

void *pfs_init(struct fuse_conn_info *conn)
{
    return;
}

int pfs_release(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;

    retstat = close(fi->fh);

    return retstat;
}

//Start FUSE code again
static struct fuse_operations pfs_oper = {
        .getattr    = pfs_getattr,
        .readdir    = pfs_readdir,
        .open       = pfs_open,
        .read       = pfs_read,
        .write      = pfs_write,
        .unlink     = pfs_unlink,
        .create     = pfs_create,
        .init       = pfs_init,
        .destroy    = pfs_destroy,
        .release    = pfs_release,
};

int main(int argc, char *argv[])
{
    FILE* dirlist;

    //Init tmp directories for DB communication
    mkdir("/tmp/rpfs", 0777);
    mkdir("/tmp/rpfs/pyreadpath", 0777); //path for requested files placed here by FUSE, title is file to read
    mkdir("/tmp/rpfs/read", 0777); //files to read placed here by python script
    mkdir("/tmp/rpfs/write", 0777); //files to write placed here by FUSE
    mkdir("/tmp/rpfs/dir", 0777); //files with list of file names and size placed here by python script
    mkdir("/tmp/rpfs/unlink", 0777); //files to remove placed here by FUSE, title is file to remove
    dirlist = fopen("/tmp/rpfs/dir/dirlist.txt", "w");
    fclose(dirlist);
    mkdir("/tmp/rpfs/stats", 0777);
    fclose(fopen("/tmp/rpfs/stats/getattr.txt", "w")); // create statistics files
    fclose(fopen("/tmp/rpfs/stats/open.txt", "w"));
    fclose(fopen("/tmp/rpfs/stats/read.txt", "w"));
    fclose(fopen("/tmp/rpfs/stats/readdir.txt", "w"));
    fclose(fopen("/tmp/rpfs/stats/write.txt", "w"));
    fclose(fopen("/tmp/rpfs/stats/create.txt", "w"));

    if ((argc < 2) || (argv[argc - 1][0] == '-')) // abort if there are less than 2 provided argument or if the path starts with a hyphen (breaks)
    {
        abort();
    }

    return fuse_main(argc, argv, &pfs_oper, NULL);
}
