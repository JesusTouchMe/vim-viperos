// Copyright 2025 JesusTouchMe

#ifndef FILEBUF_H
#define FILEBUF_H 1

struct line_buffer {
    char* buf;
    int len;
    int gap_start;
    int gap_end;
};

struct file_buffer {
    int fd;

    struct line_buffer* lines;
    int line_count;
};

int open_file_buffer(struct file_buffer* fb, const char* path);
void close_file_buffer(struct file_buffer* fb); // not auto save! call save_file_buffer first!!!!!

int save_file_buffer(struct file_buffer* fb);

// to have shorter names for these i will prefix them 'fb' short for 'file_buffer'

char fb_char_at(struct file_buffer* fb, int line, int col); // 0 if out of bounds

int fb_line_length(struct file_buffer* fb, int line); // -1 is out of bounds

void fb_set_cursor_pos(struct file_buffer* fb, int line, int col); // clamp if out of bounds

void fb_delete_char(struct file_buffer* fb, int line); // silently ignore if out of bounds

#endif //FILEBUF_H