//Modified version of FUSE filesystem you create in the FUSE tutorial here: http://www.cs.nmsu.edu/~pfeiffer/fuse-tutorial/

#include "params.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/xattr.h>

int __mkdir(const char *);
int _mkdir(const char *, mode_t);

static struct ytfs_state *ytfs_data = NULL;

int ytfs_error(char *str)
{
    int ret = -errno;
    
    return ret;
}

void ytfs_fullpath(char fpath[PATH_MAX], const char *path)
{
    strcpy(fpath, ytfs_data->rootdir);
    strncat(fpath, path, PATH_MAX); 
}

int ytfs_getattr(const char *path, struct stat *statbuf)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    
    ytfs_fullpath(fpath, path);
    
    retstat = lstat(fpath, statbuf);
    if (retstat != 0)
	retstat = ytfs_error("ytfs_getattr lstat");
    
    return retstat;
}

int ytfs_readlink(const char *path, char *link, size_t size)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    
    ytfs_fullpath(fpath, path);
    
    retstat = readlink(fpath, link, size);
    if (retstat < 0)
	retstat = ytfs_error("ytfs_readlink readlink");
    
    return 0;
}

int ytfs_mknod(const char *path, mode_t mode, dev_t dev)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    
    ytfs_fullpath(fpath, path);
    
    if (S_ISREG(mode)) {
        retstat = open(fpath, O_CREAT | O_EXCL | O_WRONLY, mode);
	if (retstat < 0)
	    retstat = ytfs_error("ytfs_mknod open");
        else {
            retstat = close(retstat);
	    if (retstat < 0)
		retstat = ytfs_error("ytfs_mknod close");
	}
    } else
	if (S_ISFIFO(mode)) {
	    retstat = mkfifo(fpath, mode);
	    if (retstat < 0)
		retstat = ytfs_error("ytfs_mknod mkfifo");
	} else {
	    retstat = mknod(fpath, mode, dev);
	    if (retstat < 0)
		retstat = ytfs_error("ytfs_mknod mknod");
	}
    
    return retstat;
}


int ytfs_mkdir(const char *path, mode_t mode)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    
    ytfs_fullpath(fpath, path);
    
    retstat = _mkdir(fpath, mode);
    if (retstat < 0)
	retstat = ytfs_error("ytfs_mkdir mkdir");
    
    return retstat;
}

int __mkdir(const char *path) {
    return _mkdir(path, S_IRWXU);
}

int _mkdir(const char *path, mode_t mode)
{
        char opath[256];
        char *p;
        size_t len;
        int res;

        strncpy(opath, path, sizeof(opath));
        len = strlen(opath);
        if(opath[len - 1] == '/')
                opath[len - 1] = '\0';
        for(p = opath; *p; p++)
                if(*p == '/') {
                        *p = '\0';
                        if(access(opath, F_OK))
                                res = mkdir(opath, mode);
                        *p = '/';
                }
        if(access(opath, F_OK))         
                res = mkdir(opath, mode);
        return res;
}

int ytfs_unlink(const char *path)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    
    ytfs_fullpath(fpath, path);
    
    retstat = unlink(fpath);
    if (retstat < 0)
	retstat = ytfs_error("ytfs_unlink unlink");
    
    return retstat;
}

int ytfs_rmdir(const char *path)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    
    ytfs_fullpath(fpath, path);
    
    retstat = rmdir(fpath);
    if (retstat < 0)
	retstat = ytfs_error("ytfs_rmdir rmdir");
    
    return retstat;
}

int ytfs_symlink(const char *path, const char *link)
{
    int retstat = 0;
    char flink[PATH_MAX];
    
    ytfs_fullpath(flink, link);
    
    retstat = symlink(path, flink);
    if (retstat < 0)
	retstat = ytfs_error("ytfs_symlink symlink");
    
    return retstat;
}

int ytfs_rename(const char *path, const char *newpath)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    char fnewpath[PATH_MAX];
    
    ytfs_fullpath(fpath, path);
    ytfs_fullpath(fnewpath, newpath);
    
    retstat = rename(fpath, fnewpath);
    if (retstat < 0)
	retstat = ytfs_error("ytfs_rename rename");
    
    return retstat;
}

int ytfs_link(const char *path, const char *newpath)
{
    int retstat = 0;
    char fpath[PATH_MAX], fnewpath[PATH_MAX];
    
    ytfs_fullpath(fpath, path);
    ytfs_fullpath(fnewpath, newpath);
    
    retstat = link(fpath, fnewpath);
    if (retstat < 0)
	retstat = ytfs_error("ytfs_link link");
    
    return retstat;
}

int ytfs_chmod(const char *path, mode_t mode)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    
    ytfs_fullpath(fpath, path);
    
    retstat = chmod(fpath, mode);
    if (retstat < 0)
	retstat = ytfs_error("ytfs_chmod chmod");
    
    return retstat;
}

int ytfs_chown(const char *path, uid_t uid, gid_t gid)
  
{
    int retstat = 0;
    char fpath[PATH_MAX];
    
    ytfs_fullpath(fpath, path);
    
    retstat = chown(fpath, uid, gid);
    if (retstat < 0)
	retstat = ytfs_error("ytfs_chown chown");
    
    return retstat;
}

int ytfs_truncate(const char *path, off_t newsize)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    
    ytfs_fullpath(fpath, path);
    
    retstat = truncate(fpath, newsize);
    if (retstat < 0)
	ytfs_error("ytfs_truncate truncate");
    
    return retstat;
}

int ytfs_utime(const char *path, struct utimbuf *ubuf)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    
    ytfs_fullpath(fpath, path);
    
    retstat = utime(fpath, ubuf);
    if (retstat < 0)
	retstat = ytfs_error("ytfs_utime utime");
    
    return retstat;
}

int ytfs_open(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    int fd;
    char fpath[PATH_MAX];
    
    ytfs_fullpath(fpath, path);
    
    fd = open(fpath, fi->flags);
    if (fd < 0)
	retstat = ytfs_error("ytfs_open open");
    
    fi->fh = fd;
    
    return 0;
}

int ytfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    int retstat = 0;
    
    retstat = pread(fi->fh, buf, size, offset);
    if (retstat < 0)
	retstat = ytfs_error("ytfs_read read");
    
    return retstat;
}

int ytfs_write(const char *path, const char *buf, size_t size, off_t offset,
	     struct fuse_file_info *fi)
{
    int retstat = 0;	
    retstat = pwrite(fi->fh, buf, size, offset);
    if (retstat < 0)
	retstat = ytfs_error("ytfs_write pwrite");
    
    return retstat;
}

int ytfs_statfs(const char *path, struct statvfs *statv)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    
    ytfs_fullpath(fpath, path);
    retstat = statvfs(fpath, statv);
    if (retstat < 0)
	retstat = ytfs_error("ytfs_statfs statvfs");
    
    
    return retstat;
}

int ytfs_flush(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    
	
    return retstat;
}

int ytfs_release(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    
    // TODO
	// Release is the last call after a file is added to the system
	// This will be where we examine the ID3 data and sort the file accordingly
	// TODO add id3lib and implement sorting functionality
        
    return retstat;
}

int ytfs_fsync(const char *path, int datasync, struct fuse_file_info *fi)
{
    int retstat = 0;
    
    
    if (datasync)
	retstat = fdatasync(fi->fh);
    else
	retstat = fsync(fi->fh);
    
    if (retstat < 0)
	ytfs_error("ytfs_fsync fsync");
    
    return retstat;
}

int ytfs_setxattr(const char *path, const char *name, const char *value, size_t size, int flags)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    
    ytfs_fullpath(fpath, path);
    
    retstat = lsetxattr(fpath, name, value, size, flags);
    if (retstat < 0)
	retstat = ytfs_error("ytfs_setxattr lsetxattr");
    
    return retstat;
}

int ytfs_getxattr(const char *path, const char *name, char *value, size_t size)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    
    ytfs_fullpath(fpath, path);
    
    retstat = lgetxattr(fpath, name, value, size);
    if (retstat < 0)
	retstat = ytfs_error("ytfs_getxattr lgetxattr");
    
    return retstat;
}

int ytfs_listxattr(const char *path, char *list, size_t size)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    
    ytfs_fullpath(fpath, path);
    
    retstat = llistxattr(fpath, list, size);
    if (retstat < 0)
	retstat = ytfs_error("ytfs_listxattr llistxattr");
    
    
    return retstat;
}

int ytfs_removexattr(const char *path, const char *name)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    
    ytfs_fullpath(fpath, path);
    
    retstat = lremovexattr(fpath, name);
    if (retstat < 0)
	retstat = ytfs_error("ytfs_removexattr lrmovexattr");
    
    return retstat;
}

int ytfs_opendir(const char *path, struct fuse_file_info *fi)
{
    DIR *dp;
    int retstat = 0;
    char fpath[PATH_MAX];
    
    ytfs_fullpath(fpath, path);
    
    dp = opendir(fpath);
    if (dp == NULL)
	retstat = ytfs_error("ytfs_opendir opendir");
    
    fi->fh = (intptr_t) dp;
    
    
    return retstat;
}


int ytfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
	       struct fuse_file_info *fi)
{
    int retstat = 0;
    DIR *dp;
    struct dirent *de;
    
    dp = (DIR *) (uintptr_t) fi->fh;

    de = readdir(dp);
    if (de == 0)
	return -errno;

    do {
	if (filler(buf, de->d_name, NULL, 0) != 0)
	    return -ENOMEM;
    } while ((de = readdir(dp)) != NULL);
    
    
    return retstat;
}

int ytfs_releasedir(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    
    
    closedir((DIR *) (uintptr_t) fi->fh);
    
    return retstat;
}

int ytfs_fsyncdir(const char *path, int datasync, struct fuse_file_info *fi)
{
    int retstat = 0;
    
    return retstat;
}

void *ytfs_init(struct fuse_conn_info *conn)
{
    return YTFS_DATA;
}


void ytfs_destroy(void *userdata)
{
}

int ytfs_access(const char *path, int mask)
{
    int retstat = 0;
    char fpath[PATH_MAX];
   
    ytfs_fullpath(fpath, path);
    
    retstat = access(fpath, mask);
    
    if (retstat < 0)
	retstat = ytfs_error("ytfs_access access");
    
    return retstat;
}

int ytfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    int fd;
    
    ytfs_fullpath(fpath, path);
    
    fd = creat(fpath, mode);
    if (fd < 0)
	retstat = ytfs_error("ytfs_create creat");
    
    fi->fh = fd;
    
    return retstat;
}

int ytfs_ftruncate(const char *path, off_t offset, struct fuse_file_info *fi)
{
    int retstat = 0;
    
    
    retstat = ftruncate(fi->fh, offset);
    if (retstat < 0)
	retstat = ytfs_error("ytfs_ftruncate ftruncate");
    
    return retstat;
}

int ytfs_fgetattr(const char *path, struct stat *statbuf, struct fuse_file_info *fi)
{
    int retstat = 0;
    
    
    retstat = fstat(fi->fh, statbuf);
    if (retstat < 0)
	retstat = ytfs_error("ytfs_fgetattr fstat");
    
    
    return retstat;
}

struct fuse_operations ytfs_oper = {
  .getattr = ytfs_getattr,
  /*.readlink = ytfs_readlink,
  .getdir = NULL,
  .mknod = ytfs_mknod,
  .mkdir = ytfs_mkdir,
  .unlink = ytfs_unlink,
  .rmdir = ytfs_rmdir,
  .symlink = ytfs_symlink,
  .rename = ytfs_rename,
  .link = ytfs_link,
  .chmod = ytfs_chmod,
  .chown = ytfs_chown,
  .truncate = ytfs_truncate,
  .utime = ytfs_utime,*/
  .open = ytfs_open,
  .read = ytfs_read,
  /*.write = ytfs_write,
  .statfs = ytfs_statfs,
  .flush = ytfs_flush,
  .release = ytfs_release,
  .fsync = ytfs_fsync,
  .setxattr = ytfs_setxattr,
  .getxattr = ytfs_getxattr,
  .listxattr = ytfs_listxattr,
  .removexattr = ytfs_removexattr,
  .opendir = ytfs_opendir,*/
  .readdir = ytfs_readdir,
  /*.releasedir = ytfs_releasedir,
  .fsyncdir = ytfs_fsyncdir,
  .init = ytfs_init,
  .destroy = ytfs_destroy,
  .access = ytfs_access,
  .create = ytfs_create,
  .ftruncate = ytfs_ftruncate,
  .fgetattr = ytfs_fgetattr*/
};

void ytfs_usage()
{
    fprintf(stdout, "usage:  bbfs rootDir mountPoint\n");
    abort();
}

int main(int argc, char *argv[])
{
    int i;
    int fuse_stat;

    ytfs_data = calloc(sizeof(struct ytfs_state), 1);
    if (ytfs_data == NULL) {
	    perror("main calloc");
	    abort();
    }
    
    for (i = 1; (i < argc) && (argv[i][0] == '-'); i++);
    if (i == argc)
	    ytfs_usage();
    
    ytfs_data->rootdir = realpath(argv[i], NULL);

    for (; i < argc; i++)
	   argv[i] = argv[i+1];
    argc--;

    fprintf(stdout, "about to call fuse_main\n");
    fuse_stat = fuse_main(argc, argv, &ytfs_oper, ytfs_data);
    fprintf(stdout, "fuse_main returned %d\n", fuse_stat);
    
    return fuse_stat;
}
