#include "buckets.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

/* Header layout:
   offset 0: magic 4 bytes // Borrar por simplicidad
   offset 4: version uint16 //
   offset 6: reserved uint16 //
   offset 8: page_size uint32
   offset 12: num_buckets uint64
   offset 20: hash_seed uint64
   offset 28: entry_size uint32 (8)
   rest: padding to BUCKETS_HEADER_SIZE
*/

int buckets_create(const char *path, uint64_t num_buckets, uint64_t hash_seed) {
    mkdir("data/index", 0755); 
    int fd = open(path, O_CREAT | O_TRUNC | O_RDWR, 0644);
    if (fd < 0) {
        printf("open %s failed: %s\n", path, strerror(errno));
        return -1;
    }

    unsigned char header[BUCKETS_HEADER_SIZE];
    memset(header, 0, sizeof(header));


    if (safe_pwrite(fd, header, BUCKETS_HEADER_SIZE, 0) != (ssize_t)BUCKETS_HEADER_SIZE) {
        close(fd);
        return -1;
    }

    /* write zeroed bucket entries */
    off_t entries_offset = BUCKETS_HEADER_SIZE;
    size_t entries_size = (size_t)num_buckets * BUCKET_ENTRY_SIZE;
    /* allocate zero buffer in chunks to avoid huge malloc */
    size_t chunk = 65536;
    unsigned char *zeros = calloc(1, chunk);
    if (!zeros) {
        close(fd);
        return -1;
    }

    size_t remaining = entries_size;
    off_t pos = entries_offset;
    while (remaining > 0) {
        size_t towrite = remaining < chunk ? remaining : chunk;
        if (safe_pwrite(fd, zeros, towrite, pos) != (ssize_t)towrite) {
            free(zeros);
            close(fd);
            return -1;
        }
        pos += towrite;
        remaining -= towrite;
    }
    free(zeros);
    fsync(fd);
    lseek(fd, 0, SEEK_SET);
    close(fd);
    return 0;
}

int buckets_open_readwrite(const char *path, uint64_t *num_buckets_out, uint64_t *hash_seed_out) {
    int fd = open(path, O_RDWR);
    if (fd < 0) return -1;
    unsigned char header[BUCKETS_HEADER_SIZE];
    if (safe_pread(fd, header, BUCKETS_HEADER_SIZE, 0) != (ssize_t)BUCKETS_HEADER_SIZE) {
        close(fd);
        return -1;
    }
    uint64_t num_buckets, hash_seed;
    memcpy(&num_buckets, header + 12, sizeof num_buckets);
    memcpy(&hash_seed,   header + 20, sizeof hash_seed);
    if (num_buckets_out) *num_buckets_out = num_buckets;
    if (hash_seed_out) *hash_seed_out = hash_seed;
    return fd;
}

off_t buckets_entry_offset(uint64_t bucket_id) {
    return (off_t)BUCKETS_HEADER_SIZE + (off_t)bucket_id * BUCKET_ENTRY_SIZE;
}

off_t buckets_read_head(int fd, uint64_t num_buckets, uint64_t bucket_id) {
    if (bucket_id >= num_buckets) return 0;
    off_t pos = buckets_entry_offset(bucket_id);
    unsigned char buf[8];
    if (safe_pread(fd, buf, 8, pos) != 8) return 0;
    uint64_t v;
    memcpy(&v, buf, sizeof v);
    return (off_t)v;
}

int buckets_write_head(int fd, uint64_t num_buckets, uint64_t bucket_id, off_t head) {
    if (bucket_id >= num_buckets) return -1;
    off_t pos = buckets_entry_offset(bucket_id);
    if (safe_pwrite(fd, &head, 8, pos) != 8) return -1;
    return 0;
}
