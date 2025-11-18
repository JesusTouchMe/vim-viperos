// Copyright 2025 JesusTouchMe

#include "filebuf.h"
#include "tui.h"

#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

static struct termios oldt;

void enable_raw_mode(void) {
    tcgetattr(STDIN_FILENO, &oldt);
    struct termios newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    newt.c_cc[VMIN] = 1;
    newt.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
}

void disable_raw_mode(void) {
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}

ssize_t read_nonblocking(int fd, char* buf, size_t size) {
    fd_set set;
    struct timeval tv = { .tv_sec = 0, .tv_usec = 50000 };

    FD_ZERO(&set);
    FD_SET(fd, &set);

    int ready = select(fd + 1, &set, NULL, NULL, &tv);
    if (ready > 0) {
        return read(fd, buf, size);
    }

    return 0;
}

void cleanupatexit() {
    tui_destroy();
    disable_raw_mode();
}

void termneatly(int sig) {
    exit(67);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        return 1;
    }

    enable_raw_mode();
    tui_init();

    atexit(cleanupatexit);

    signal(SIGINT, SIG_IGN);
    signal(SIGTERM, termneatly);

    struct file_buffer fb;
    if (open_file_buffer(&fb, argv[1]) != 0) {
        return 1;
    }

    int scroll = 0;
    int cursor_line = 0;
    int cursor_col = 0;

    char input_buf[8];
    while (1) {
        bool op_dl = false; // vim operator 'dl' with 'x' as a shortcut for it. for now, we only support the shortcut

        int n = read_nonblocking(STDIN_FILENO, input_buf, sizeof(input_buf));
        if (n > 0) {
            if (input_buf[0] == 'q') break;

            if (input_buf[0] == 'h') {
                if (cursor_col > 0)
                    cursor_col--;
            } else if (input_buf[0] == 'l') {
                int len = fb_line_length(&fb, cursor_line);
                if (cursor_col < len)
                    cursor_col++;
            } else if (input_buf[0] == 'j') {
                if (cursor_line < fb.line_count - 1) {
                    cursor_line++;
                    int len = fb_line_length(&fb, cursor_line);
                    if (cursor_col > len) cursor_col = len;
                }
            } else if (input_buf[0] == 'k') {
                if (cursor_line > 0) {
                    cursor_line--;
                    int len = fb_line_length(&fb, cursor_line);
                    if (cursor_col > len) cursor_col = len;
                }
            } else if (input_buf[0] == 'x') {
                op_dl = true;
            }
        }

        int line_len_current = fb_line_length(&fb, cursor_line);
        if (cursor_col > line_len_current) cursor_col = line_len_current;
        if (cursor_col < 0) cursor_col = 0;

        fb_set_cursor_pos(&fb, cursor_line, cursor_col);

        if (op_dl) fb_delete_char(&fb, cursor_line);

        struct dimensions screen_size = tui_get_screen_size();

        if (scroll < 0) scroll = 0;
        int max_scroll = fb.line_count - screen_size.height;
        if (scroll > max_scroll) scroll = max_scroll;

        int max_chars = screen_size.width - 5;
        if (max_chars < 1) max_chars = 1;

        tui_clear();

        int global_cursor_y = 0;
        for (int j = 0; j < cursor_line; j++) {
            int len = fb_line_length(&fb, j);
            if (len <= 0) {
                global_cursor_y += 1;
            } else {
                global_cursor_y += (len + max_chars - 1) / max_chars;
            }
        }
        global_cursor_y += cursor_col / max_chars;

        int visual_cursor_x = 5 + (cursor_col % max_chars);
        int visual_cursor_y = global_cursor_y - scroll;

        if (visual_cursor_y >= screen_size.height)
            scroll = global_cursor_y - screen_size.height + 1;

        if (visual_cursor_y < 0)
            scroll = global_cursor_y;

        int y = 0;
        for (int i = scroll; i < fb.line_count && y < screen_size.height; i++) {
            int line_len = fb_line_length(&fb, i);
            int start = 0;

            while ((start < line_len || start == 0) && y < screen_size.height) {
                if (start == 0) {
                    char numbuf[8];
                    snprintf(numbuf, sizeof(numbuf), "%-4d", i + 1);
                    for (int x = 0; numbuf[x] != 0 && x < screen_size.width; x++) {
                        tui_put(x, y, numbuf[x]);
                    }
                }

                for (int x = 0; x < max_chars && start + x < line_len; x++) {
                    tui_put(x + 5, y, fb_char_at(&fb, i, start + x));
                }

                start += max_chars;
                y++;
            }
        }

        tui_set_invert(visual_cursor_x, visual_cursor_y, true);

        tui_render();
    }

    save_file_buffer(&fb);
    close_file_buffer(&fb);

    return 0;
}