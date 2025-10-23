
#define _XOPEN_SOURCE 500

#include "common.h"
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>

ssize_t safe_pread(int fd, void *buf, size_t count, off_t offset) {
    ssize_t total = 0;
    char *p = (char*)buf;
    while (total < (ssize_t)count) {
        ssize_t r = pread(fd, p + total, count - total, offset + total);
        if (r < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (r == 0) break;
        total += r;
    }
    return total;
}

ssize_t safe_pwrite(int fd, const void *buf, size_t count, off_t offset) {
    ssize_t total = 0;
    const char *p = (const char*)buf;
    while (total < (ssize_t)count) {
        ssize_t w = pwrite(fd, p + total, count - total, offset + total);
        if (w < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        total += w;
    }
    return total;
}

