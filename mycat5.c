#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#define OPTIMAL_BLOCKSIZE (32 * 1024) // 32KB

char* align_alloc(size_t size) {
    void* ptr = NULL;
    size_t alignment = 4096; // 4K页对齐
    if (posix_memalign(&ptr, alignment, size) != 0) {
        return NULL;
    }
    return (char*)ptr;
}

void align_free(void* ptr) {
    free(ptr);
}

size_t io_blocksize(int fd) {
    return OPTIMAL_BLOCKSIZE;
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