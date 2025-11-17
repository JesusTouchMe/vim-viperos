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
        int n = read_nonblocking(STDIN_FILENO, input_buf, sizeof(input_buf));
        if (n > 0) {
            if (input_buf[0] == 'q') break;

            if (input_buf[0] == 'h') {
                if (cursor_col > 0) cursor_col--;
            } else if (input_buf[0] == 'l') {
                if (cursor_col < (int)strlen(fb.lines[cursor_line]) - 1)
                    cursor_col++;
            } else if (input_buf[0] == 'j') {
                if (cursor_line < fb.line_count - 1) {
                    cursor_line++;
                    if (cursor_col > (int) strlen(fb.lines[cursor_line]))
                        cursor_col = strlen(fb.lines[cursor_line]);
                }
            } else if (input_buf[0] == 'k') {
                if (cursor_line > 0) {
                    cursor_line--;
                    if (cursor_col > (int) strlen(fb.lines[cursor_line]))
                        cursor_col = strlen(fb.lines[cursor_line]);
                }
            }
        }

        if (cursor_col >= strlen(fb.lines[cursor_line])) {
            cursor_col = strlen(fb.lines[cursor_line]) - 1;
        }

        struct dimensions screen_size = tui_get_screen_size();

        /*
        if (cursor_line >= scroll + screen_size.height)
            scroll = cursor_line - screen_size.height + 1;

        if (cursor_line < scroll)
            scroll = cursor_line;
            */

        if (scroll < 0) scroll = 0;
        int max_scroll = fb.line_count - screen_size.height;
        if (scroll > max_scroll) scroll = max_scroll;

        tui_clear();

        int max_chars = screen_size.width - 5;

        int global_cursor_y = 0;
        for (int j = 0; j < cursor_line; j++) {
            size_t len = strlen(fb.lines[j]);
            global_cursor_y += (len + max_chars - 1) / max_chars;
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
            const char* line = fb.lines[i];
            size_t line_len = strlen(line);

            int start = 0;
            while (start < line_len && y < screen_size.height) {
                if (start == 0) {
                    char numbuf[8];
                    snprintf(numbuf, sizeof(numbuf), "%-4d", i + 1);
                    for (int x = 0; numbuf[x] != 0 && x < screen_size.width; x++) {
                        tui_put(x, y, numbuf[x]);
                    }
                }

                for (int x = 0; x < max_chars && start + x < line_len; x++) {
                    tui_put(x + 5, y, line[start + x]);
                }

                start += max_chars;
                y++;
            }
        }

        tui_set_invert(visual_cursor_x, visual_cursor_y, true);

        tui_render();
    }

    close_file_buffer(&fb);

    return 0;
}