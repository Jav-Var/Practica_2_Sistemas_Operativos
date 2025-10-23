#include "arrays.h"
#include "common.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>

/* create arrays.dat and reserve header bytes */
int arrays_create(const char *path) {
    int fd = open(path, O_CREAT | O_TRUNC | O_RDWR, 0644);
    /* O_CREAT: creates the file if it doesn't exist.
     * O_TRUNC: if it exists, it truncates it to length 0 (deletes its content).
     * O_RDWR: opens it in read/write mode (necessary for pread/pwrite).
     */
    if (fd < 0) return -1;
    unsigned char header[ARRAYS_HEADER_SIZE];
    memset(header, 0, ARRAYS_HEADER_SIZE);
    if (safe_pwrite(fd, header, ARRAYS_HEADER_SIZE, 0) != (ssize_t)ARRAYS_HEADER_SIZE) {
        close(fd);
        return -1;
    }
    fsync(fd);
    close(fd);
    return 0;
}

int arrays_open(const char *path) {
    int fd = open(path, O_RDWR);
    return fd;
}

/* returns the size of a node with a key (string) of size key_len, and a list of offsets of size list_len */
size_t arrays_calc_node_size(uint16_t key_len, uint32_t list_len) {
    return sizeof(uint16_t)                  // key_len
         + (size_t)key_len                   // key bytes
         + sizeof(uint32_t)                  // list_len
         + (size_t)list_len * sizeof(uint64_t) // offsets[]
         + sizeof(uint64_t);                 // next_ptr
}

/* append node: serialize into a buffer then write at EOF */
off_t arrays_append_node(int fd, const arrays_node_t *node) {
    if (!node || !node->key) return 0;
    uint16_t key_len = (uint16_t)strlen(node->key);
    if ((size_t)key_len != strlen(node->key)) {
        /* key too long for uint16 */
        return 0;
    }
    uint32_t list_len = node->list_len;

    size_t node_size = arrays_calc_node_size(key_len, list_len);
    unsigned char *buf = malloc(node_size);
    if (!buf) return 0;
    size_t pos = 0;

    /* key_len */
    memcpy(buf + pos, &key_len, 2);
    pos += sizeof(uint16_t);

    /* key bytes (no NUL on disk) */
    memcpy(buf + pos, node->key, key_len);
    pos += key_len;

    /* list_len */
    memcpy(buf + pos, &list_len, 4);
    pos += sizeof(uint32_t);

    /* offsets array */
    for (uint32_t i = 0; i < list_len; ++i) {
        memcpy(buf + pos, &node->offsets[i], sizeof node->offsets[i]);
        pos += sizeof node->offsets[i];
    }

    /* next_ptr */
    memcpy(buf + pos, &node->next_ptr, sizeof node->next_ptr);
    pos += sizeof(uint64_t);

    /* compute offset at EOF */
    off_t new_off = lseek(fd, 0, SEEK_END);
    if (new_off == (off_t)-1) {
        free(buf);
        return 0;
    }
    /* ensure we don't write into header area */
    if (new_off < (off_t)ARRAYS_HEADER_SIZE) new_off = (off_t)ARRAYS_HEADER_SIZE;

    if (safe_pwrite(fd, buf, node_size, new_off) != (ssize_t)node_size) {
        free(buf);
        return 0;
    }

    free(buf);
    /* optional fsync for durability */
    return (off_t)new_off;
}

// reads the node data in an offset to a struct (arrays_node_t)
int arrays_read_node_full(int fd, off_t node_off, arrays_node_t *node) {
    if (!node) return -1;
    uint16_t key_len;
    if (safe_pread(fd, &key_len, sizeof(uint16_t), node_off) != (ssize_t)sizeof(uint16_t)) return -1;

    /* read key bytes and list_len (key_len + sizeof(list_len)) */
    size_t head_read = (size_t)key_len + sizeof(uint32_t);
    unsigned char *headbuf = malloc(head_read);
    if (!headbuf) return -1;
    if (safe_pread(fd, headbuf, head_read, node_off + sizeof(uint16_t)) != (ssize_t)head_read) {
        free(headbuf);
        return -1;
    }

    /* allocate key (NUL-terminated) */
    char *key = malloc((size_t)key_len + 1);
    if (!key) { free(headbuf); return -1; }
    memcpy(key, headbuf, key_len);
    key[key_len] = '\0';

    uint32_t list_len;
    memcpy(&list_len, headbuf + key_len, sizeof list_len);
    free(headbuf);

    /* read offsets and next_ptr */
    size_t rest_bytes = (size_t)list_len * sizeof(uint64_t) + sizeof(uint64_t);
    unsigned char *restbuf = NULL;
    if (rest_bytes > 0) {
        restbuf = malloc(rest_bytes);
        if (!restbuf) { free(key); return -1; }
        if (safe_pread(fd, restbuf, rest_bytes, node_off + sizeof(uint16_t) + key_len + sizeof(uint32_t)) != (ssize_t)rest_bytes) {
            free(key); free(restbuf); return -1;
        }
    } else {
        restbuf = malloc(sizeof(uint64_t));
        if (!restbuf) { free(key); return -1; }
        if (safe_pread(fd, restbuf, sizeof(uint64_t), node_off + sizeof(uint16_t) + key_len + sizeof(uint32_t)) != (ssize_t)sizeof(uint64_t)) {
            free(key); free(restbuf); return -1;
        }
    }

    off_t *offsets = NULL;
    if (list_len > 0) {
        offsets = malloc(sizeof(off_t) * list_len);
        if (!offsets) { free(key); free(restbuf); return -1; }
        for (uint32_t i = 0; i < list_len; ++i) {
            memcpy(&offsets[i], restbuf + (size_t)i * 8, sizeof offsets[i]);
        }
    }

    uint64_t tmp;
    memcpy(&tmp, restbuf + (size_t)list_len * sizeof(uint64_t), sizeof tmp);
    off_t next = (off_t)tmp;
    free(restbuf);

    /* fill node struct */
    node->key_len = key_len;
    node->key = key;
    node->list_len = list_len;
    node->offsets = offsets;
    node->next_ptr = next;
    return 0;
}


void arrays_free_node(arrays_node_t *node) {
    if (!node) return;
    if (node->key) { free(node->key); node->key = NULL; }
    if (node->offsets) { free(node->offsets); node->offsets = NULL; }
    node->key_len = 0;
    node->list_len = 0;
    node->next_ptr = 0;
}