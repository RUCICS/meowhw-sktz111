#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

// 获取系统内存页大小
size_t get_pagesize() {
    long sz = sysconf(_SC_PAGESIZE);
    if (sz <= 0) return 4096;
    return (size_t)sz;
}

// 获取文件系统块大小
size_t get_filesystem_blocksize(int fd) {
    struct stat st;
    if (fstat(fd, &st) == 0) {
        // st_blksize 可能不是2的幂，需判断
        size_t blk = (size_t)st.st_blksize;
        // 简单判断是否为2的幂
        if (blk >= 512 && (blk & (blk - 1)) == 0) {
            return blk;
        }
    }
    // 获取失败或不合理时，返回0
    return 0;
}

// 取内存页大小和文件系统块大小的较大值作为缓冲区大小
size_t io_blocksize(int fd) {
    size_t pagesize = get_pagesize();
    size_t blksize = get_filesystem_blocksize(fd);
    if (blksize == 0) return pagesize;
    // 取较大的那个
    return pagesize > blksize ? pagesize : blksize;
}

// 分配对齐到内存页的缓冲区
char* align_alloc(size_t size) {
    void* ptr = NULL;
    size_t alignment = get_pagesize();
    if (posix_memalign(&ptr, alignment, size) != 0) {
        return NULL;
    }
    return (char*)ptr;
}

void align_free(void* ptr) {
    free(ptr);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "用法: %s <文件名>\n", argv[0]);
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("打开文件失败");
        return 1;
    }

    size_t bufsize = io_blocksize(fd);
    char *buf = align_alloc(bufsize);
    if (!buf) {
        perror("分配缓冲区失败");
        close(fd);
        return 1;
    }

    ssize_t n;
    while ((n = read(fd, buf, bufsize)) > 0) {
        ssize_t written = 0;
        while (written < n) {
            ssize_t w = write(STDOUT_FILENO, buf + written, n - written);
            if (w < 0) {
                perror("写入失败");
                align_free(buf);
                close(fd);
                return 1;
            }
            written += w;
        }
    }
    if (n < 0) {
        perror("读取失败");
        align_free(buf);
        close(fd);
        return 1;
    }

    align_free(buf);
    close(fd);
    return 0;
}