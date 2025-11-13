// Copyright 2025 JesusTouchMe

#include "filebuf.h"

#include <sys/file.h>

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

static char* read_all(int fd, size_t* out_size) {
    size_t cap = 4096, size = 0;
    char* buf = malloc(cap + 1);
    if (buf == NULL) {
        close(fd);
        return NULL;
    }

    while (1) {
        ssize_t n = read(fd, buf + size, cap - size);
        if (n < 0) {
            free(buf);
            close(fd);
            return NULL;
        }
        if (n == 0) break;
        size += n;
        if (size == cap) {
            cap *= 2;
            char* tmp = realloc(buf, cap + 1);
            if (tmp == NULL) {
                free(buf);
                close(fd);
                return NULL;
            }
            buf = tmp;
        }
    }
    buf[size] = '\0';
    *out_size = size;
    return buf;
}

int open_file_buffer(struct file_buffer* fb, const char* path) {
    fb->fd = open(path, O_RDWR | O_CREAT, 0644);
    if (fb->fd < 0) return -1;

    size_t size;
    char* text = read_all(fb->fd, &size);
    if (text == NULL) {
        close(fb->fd);
        return -1;
    }

    fb->line_count = 0;
    for (size_t i = 0; i < size; i++) {
        if (text[i] == '\n') fb->line_count++;
    }

    fb->lines = malloc(fb->line_count * sizeof(char*));
    if (fb->lines == NULL) {
        free(text);
        close(fb->fd);
        return -1;
    }

    size_t line_start = 0;
    size_t line_idx = 0;

    for (size_t i = 0; i < size; i++) {
        if (i == size || text[i] == '\n') {
            size_t len = i - line_start;
            fb->lines[line_idx] = malloc(len + 1);
            if (fb->lines[line_idx] == NULL) {
                for (size_t j = 0; j < line_idx; j++) free(fb->lines[j]);
                free(fb->lines);
                free(text);
                close(fb->fd);
                return -1;
            }
            memcpy(fb->lines[line_idx], text + line_start, len);
            fb->lines[line_idx][len] = '\0';
            line_idx++;
            line_start = i + 1;
        }
    }

    free(text);
    return 0;
}

void close_file_buffer(struct file_buffer* fb) {
    for (int i = 0; i < fb->line_count; i++) free(fb->lines[i]);
    free(fb->lines);

    if (fb->fd >= 0) {
        close(fb->fd);
    }

    fb->lines = NULL;
    fb->line_count = 0;
    fb->fd = -1;
}

int save_file_buffer(struct file_buffer* fb) {
    if (fb->fd < 0 || fb->lines == NULL) return -1;

    if (lseek(fb->fd, 0, SEEK_SET) < 0) return -1;
    if (ftruncate(fb->fd, 0) < 0) return -1;

    for (size_t i = 0; i < fb->line_count; i++) {
        size_t len = strlen(fb->lines[i]);
        if (write(fb->fd, fb->lines[i], len) != (ssize_t) len) return -1;
        if (write(fb->fd, "\n", 1) != (ssize_t) len) return -1;
    }

    fsync(fb->fd);
    return 0;
}
