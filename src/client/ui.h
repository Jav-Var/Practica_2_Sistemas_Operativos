#ifndef UI_H

#define UI_H

#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdbool.h>

#define REQ_FIFO "/tmp/index_req.fifo"
#define RSP_FIFO "/tmp/index_rsp.fifo"
#define MAX_LINE 8192

void rtrim_newline(char *s);
const char *display_or_empty(const char *s);
int write_line_fd(int fd, const char *s);
void press_enter_to_continue();
void print_record(const char *record);
void read_and_print_response(int rsp_fd);
char *getline_trimmed_stdin(void);

#endif // UI_H