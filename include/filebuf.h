// Copyright 2025 JesusTouchMe

#ifndef FILEBUF_H
#define FILEBUF_H 1

struct file_buffer {
    int fd;

    char** lines;
    int line_count;
};

int open_file_buffer(struct file_buffer* fb, const char* path);
void close_file_buffer(struct file_buffer* fb);
int save_file_buffer(struct file_buffer* fb);

#endif //FILEBUF_H