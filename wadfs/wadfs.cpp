#define FUSE_USE_VERSION 26
#include <fuse.h>
#include <cstring>
#include <errno.h>
#include <vector>
#include <string>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include "../libWad/Wad.h"
#include "../libWad/Wad.cpp"

static int getattr_callback(const char *path, struct stat *stbuf) {
    memset(stbuf, 0, sizeof(struct stat));

    Wad* wad = (Wad*)fuse_get_context()->private_data;

    if (wad->isDirectory(path)) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else if (wad->isContent(path)) {
        stbuf->st_mode = S_IFREG | 0644;
        stbuf->st_nlink = 1;
        stbuf->st_size = wad->getSize(path);
    } else {
        return -ENOENT;
    }

    return 0;
}

static int mknod_callback(const char *path, mode_t mode, dev_t rdev) {
    Wad* wad = (Wad*)fuse_get_context()->private_data;

    if (!wad) {
        return -EFAULT;
    }

    if (S_ISREG(mode)) {
        wad->createFile(path);
    } else {
        return -EINVAL;
    }

    return 0;
}

static int mkdir_callback(const char *path, mode_t mode) {
    Wad* wad = (Wad*)fuse_get_context()->private_data;

    if (!wad) {
        return -EFAULT;
    }

    wad->createDirectory(path);

    return 0;
}

static int read_callback(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    Wad* wad = (Wad*)fuse_get_context()->private_data;

    if (!wad) {
        return -EFAULT;
    }

    int res = wad->getContents(path, buf, size, offset);
    if (res < 0) {
        return -ENOENT;
    }

    return res;
}

static int write_callback(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    Wad* wad = (Wad*)fuse_get_context()->private_data;

    if (!wad) {
        return -EFAULT;
    }

    int res = wad->writeToFile(path, buf, size, offset);
    if (res < 0) {
        return -ENOENT;
    }

    if (res == 0) {
        return -EPERM;
    }

    return res;
}

static int readdir_callback(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    Wad* wad = (Wad*)fuse_get_context()->private_data;

    if (!wad) {
        return -EFAULT;
    }

    std::vector<std::string> directory;
    int res = wad->getDirectory(path, &directory);
    if (res < 0) {
        return -ENOENT;
    }

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    for (const auto &name : directory) {
        filler(buf, name.c_str(), NULL, 0);
    }

    return 0;
}

static struct fuse_operations fuse_ops = {
    .getattr = getattr_callback,
    .mknod = mknod_callback,
    .mkdir = mkdir_callback,
    .read = read_callback,
    .write = write_callback,
    .readdir = readdir_callback,
};

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <WAD file> <mount directory>" << std::endl;
        return 1;
    }

    std::string wadfile = argv[argc - 2];
    if (wadfile.at(0) != '/') {
        wadfile = std::string(get_current_dir_name()) + "/" + wadfile;
    }

    Wad* wad = Wad::loadWad(wadfile);
    if (!wad) {
        std::cerr << "Error: Could not load WAD file: " << wadfile << std::endl;
        return 1;
    }
    
    argv[argc - 2] = argv[argc - 1];
    argc--;

    return fuse_main(argc, argv, &fuse_ops, wad);
}
