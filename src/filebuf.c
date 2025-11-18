// Copyright 2025 JesusTouchMe

#include "filebuf.h"

#include <sys/file.h>

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define GAP_INIT 32

static char* read_all(int fd, size_t* out_size) {
    size_t cap = 4096;
    size_t size = 0;
    char* buf = malloc(cap + 1);
    if (buf == NULL) {
        return NULL;
    }

    while (1) {
        ssize_t n = read(fd, buf + size, cap - size);
        if (n < 0) {
            free(buf);
            return NULL;
        }
        if (n == 0) break;
        size += n;
        if (size == cap) {
            cap *= 2;
            char* tmp = realloc(buf, cap + 1);
            if (tmp == NULL) {
                free(buf);
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

    if (size == 0) fb->line_count = 1;
    else if (size > 0 && text[size - 1] != '\n') fb->line_count++;

    fb->lines = malloc(fb->line_count * sizeof(struct line_buffer));
    if (fb->lines == NULL) {
        free(text);
        close(fb->fd);
        return -1;
    }

    size_t line_start = 0;
    size_t line_idx = 0;

    for (size_t i = 0; i < size; i++) {
        if (i == size || text[i] == '\n') {
            struct line_buffer* line = &fb->lines[line_idx];
            int len = i - line_start;
            int cap = len + GAP_INIT;

            line->len = cap;
            line->buf = malloc(cap);

            if (line->buf == NULL) {
                for (size_t j = 0; j < line_idx; j++) free(fb->lines[j].buf);
                free(fb->lines);
                free(text);
                close(fb->fd);
                return -1;
            }

            if (len > 0) {
                memcpy(line->buf, text + line_start, line->len);
            }

            memset(line->buf + len, 0, cap - len);

            line->gap_start = len;
            line->gap_end = cap;

            line_idx++;
            line_start = i + 1;
        }
    }

    free(text);
    return 0;
}

void close_file_buffer(struct file_buffer* fb) {
    for (int i = 0; i < fb->line_count; i++) free(fb->lines[i].buf);
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
        struct line_buffer* line = &fb->lines[i];
        if (write(fb->fd, line->buf, line->gap_start) != (ssize_t) line->gap_start) return -1;
        if (write(fb->fd, line->buf + line->gap_end, line->len - line->gap_end) != (ssize_t) line->len - line->gap_end) return -1;
        if (write(fb->fd, "\n", 1) != 1) return -1;
    }

    fsync(fb->fd);
    return 0;
}

static int lb_line_length(struct line_buffer* line);

static char lb_char_at(struct line_buffer* line, int col) {
    int len = lb_line_length(line);
    if (col < 0 || col >= len) return 0;

    int gap_size = line->gap_end - line->gap_start;

    if (col < line->gap_start) return line->buf[col];
    return line->buf[col + gap_size];
}

char fb_char_at(struct file_buffer* fb, int line, int col) {
    if (line < 0 || line >= fb->line_count) return 0; // should i do '\0' for errors instead? does it matter?
    return lb_char_at(&fb->lines[line], col);
}

static int lb_line_length(struct line_buffer* line) {
    return line->gap_start + (line->len - line->gap_end);
}

int fb_line_length(struct file_buffer* fb, int line_n) {
    if (line_n < 0 || line_n >= fb->line_count) return -1;
    struct line_buffer* line = &fb->lines[line_n];
    return lb_line_length(line);
}

static void lb_move_gap(struct line_buffer* line, int col) {
    int len = lb_line_length(line);
    if (col < 0) col = 0;
    else if (col > len) col = len;

    if (col < line->gap_start) {
        int amount = line->gap_start - col;
        memmove(line->buf + (line->gap_end - amount), line->buf + col, amount);
        line->gap_start = col;
        line->gap_end -= amount;
    } else {
        int amount = col - line->gap_start;
        memmove(line->buf + line->gap_start, line->buf + line->gap_end, amount);
        line->gap_start += amount;
        line->gap_end += amount;
    }
}

static void lb_set_cursor_pos(struct line_buffer* line, int col) {
    int len = lb_line_length(line);
    if (col > len) col = len;

    lb_move_gap(line, col);
}

void fb_set_cursor_pos(struct file_buffer* fb, int line, int col) {
    if (line < 0) line = 0;
     else if (line >= fb->line_count) line = fb->line_count - 1;

    lb_set_cursor_pos(&fb->lines[line], col);
}

static void lb_insert_char(struct line_buffer* line, char c) {
    if (line->gap_start >= line->gap_end) {
        int gap_size = (line->len / 2) + 8;
        char* new_buf = malloc(line->len + gap_size);
        if (new_buf == NULL) return; // OOM

        memcpy(new_buf, line->buf, line->gap_start);
        int after_len = line->len - line->gap_end;
        memcpy(new_buf + line->gap_start + gap_size,
               line->buf + line->gap_end,
               after_len);

        free(line->buf);
        line->buf = new_buf;
        line->gap_end = line->gap_start + gap_size;
        line->len += gap_size;
    }

    line->buf[line->gap_start++] = c;
}

void fb_insert_char(struct file_buffer* fb, int line, char c) {
    if (line < 0 || line >= fb->line_count) return;
    lb_insert_char(&fb->lines[line], c);
}

static void lb_delete_char(struct line_buffer* line) {
    if (line->gap_end >= line->len) return;
    line->gap_end++;
}

void fb_delete_char(struct file_buffer* fb, int line) {
    if (line < 0 || line >= fb->line_count) return;
    lb_delete_char(&fb->lines[line]);
}
