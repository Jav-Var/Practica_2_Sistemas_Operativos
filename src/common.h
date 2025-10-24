#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/types.h>

#define BUCKETS_HEADER_SIZE 4096
#define ARRAYS_HEADER_SIZE 4096
#define BUCKET_ENTRY_SIZE 8
#define INDEX_MAGIC "IDX1" 
#define INDEX_VERSION 1 

#define CSV_PATH "data/dataset/books_data_test.csv"
#define INDEX_DIR "data/index"
#define NUM_DATASET_FIELDS 14
#define NUM_BUCKETS 4096
#define DEFAULT_HASH_SEED 0x12345678abcdefULL

#define KEY_PREFIX_LEN 14 // lenght for a matching search 

/* safe IO wrappers */
ssize_t safe_pread(int fd, void *buf, size_t count, off_t offset);
ssize_t safe_pwrite(int fd, const void *buf, size_t count, off_t offset);

#endif // COMMON_H
