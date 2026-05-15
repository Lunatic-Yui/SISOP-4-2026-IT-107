#define FUSE_USE_VERSION 28
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <stdlib.h>

static char source_dir[1024];

#define XOR_KEY 0x76

static void xor_crypt(char *buf, size_t size) {
    for (size_t i = 0; i < size; i++) {
        buf[i] ^= XOR_KEY;
    }
}

static void get_dir_path(char *res, const char *path) {
    sprintf(res, "%s%s", source_dir, path);
}

static void get_file_path(char *res, const char *path) {
    sprintf(res, "%s%s.enc", source_dir, path);
}

static int xmp_getattr(const char *path, struct stat *stbuf) {
    char fpath[1024];
    
    get_dir_path(fpath, path);
    if (lstat(fpath, stbuf) == 0) return 0;

    get_file_path(fpath, path);
    if (lstat(fpath, stbuf) == 0) return 0;

    return -ENOENT; 
}

static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    char fpath[1024];
    get_dir_path(fpath, path);

    DIR *dp = opendir(fpath);
    if (dp == NULL) return -errno;

    struct dirent *de;
    while ((de = readdir(dp)) != NULL) {
        char name[256];
        strcpy(name, de->d_name);

        if (de->d_type == DT_REG || de->d_type == DT_UNKNOWN) {
            size_t len = strlen(name);
            if (len > 4 && strcmp(name + len - 4, ".enc") == 0) {
                name[len - 4] = '\0';
            }
        }

        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        filler(buf, name, &st, 0);
    }
    closedir(dp);
    return 0;
}

static int xmp_mkdir(const char *path, mode_t mode) {
    char fpath[1024];
    get_dir_path(fpath, path);
    int res = mkdir(fpath, mode);
    if (res == -1) return -errno;
    return 0;
}

static int xmp_rmdir(const char *path) {
    char fpath[1024];
    get_dir_path(fpath, path);
    int res = rmdir(fpath);
    if (res == -1) return -errno;
    return 0;
}

static int xmp_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    char fpath[1024];
    get_file_path(fpath, path); 
    int res = creat(fpath, mode);
    if (res == -1) return -errno;
    fi->fh = res;
    return 0;
}

static int xmp_open(const char *path, struct fuse_file_info *fi) {
    char fpath[1024];
    get_file_path(fpath, path);
    int res = open(fpath, fi->flags);
    if (res == -1) return -errno;
    close(res);
    return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    char fpath[1024];
    get_file_path(fpath, path);
    int fd = open(fpath, O_RDONLY);
    if (fd == -1) return -errno;

    int res = pread(fd, buf, size, offset);
    if (res > 0) xor_crypt(buf, res); 

    close(fd);
    if (res == -1) return -errno;
    return res;
}

static int xmp_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    char fpath[1024];
    get_file_path(fpath, path);
    int fd = open(fpath, O_WRONLY);
    if (fd == -1) return -errno;

    char *enc_buf = malloc(size);
    memcpy(enc_buf, buf, size);
    xor_crypt(enc_buf, size); 

    int res = pwrite(fd, enc_buf, size, offset);
    free(enc_buf);
    close(fd);
    if (res == -1) return -errno;
    return res;
}

static int xmp_truncate(const char *path, off_t size) {
    char fpath[1024];
    get_file_path(fpath, path);
    int res = truncate(fpath, size);
    if (res == -1) return -errno;
    return 0;
}

static int xmp_unlink(const char *path) {
    char fpath[1024];
    get_file_path(fpath, path);
    int res = unlink(fpath);
    if (res == -1) return -errno;
    return 0;
}

static int xmp_access(const char *path, int mask) {
    char fpath[1024];
    get_dir_path(fpath, path);
    if (access(fpath, mask) == 0) return 0;

    get_file_path(fpath, path);
    if (access(fpath, mask) == 0) return 0;

    return -ENOENT;
}

static int xmp_rename(const char *from, const char *to) {
    char ffrom[1024], fto[1024];
    struct stat st;
    
    get_dir_path(ffrom, from);
    if (lstat(ffrom, &st) == 0 && S_ISDIR(st.st_mode)) {
        get_dir_path(fto, to); 
    } else {
        get_file_path(ffrom, from); 
        get_file_path(fto, to);
    }

    int res = rename(ffrom, fto);
    if (res == -1) return -errno;
    return 0;
}

static int xmp_utimens(const char *path, const struct timespec ts[2]) {
    char fpath[1024];
    struct stat st;

    get_dir_path(fpath, path);
    if (lstat(fpath, &st) != 0) {
        get_file_path(fpath, path);
    }

    struct timeval tv[2];
    tv[0].tv_sec = ts[0].tv_sec;
    tv[0].tv_usec = ts[0].tv_nsec / 1000;
    tv[1].tv_sec = ts[1].tv_sec;
    tv[1].tv_usec = ts[1].tv_nsec / 1000;

    int res = utimes(fpath, tv);
    if (res == -1) return -errno;
    return 0;
}

static struct fuse_operations xmp_oper = {
    .getattr  = xmp_getattr,
    .readdir  = xmp_readdir,
    .mkdir    = xmp_mkdir,
    .rmdir    = xmp_rmdir,
    .create   = xmp_create,
    .open     = xmp_open,
    .read     = xmp_read,
    .write    = xmp_write,
    .truncate = xmp_truncate,
    .unlink   = xmp_unlink,
    .access   = xmp_access,
    .rename   = xmp_rename,
    .utimens  = xmp_utimens, 
};

int main(int argc, char *argv[]) {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd() error");
        return 1;
    }

    snprintf(source_dir, sizeof(source_dir), "%s/encrypted_storage", cwd);
    mkdir(source_dir, 0777); 

    char mount_dir[1024];
    snprintf(mount_dir, sizeof(mount_dir), "%s/fuse_mount", cwd);
    mkdir(mount_dir, 0777); 

    char *fuse_argv[] = { argv[0], mount_dir };
    int fuse_argc = 2;

    return fuse_main(fuse_argc, fuse_argv, &xmp_oper, NULL);
}