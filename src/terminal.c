// Copyright 2025 JesusTouchMe

#include "terminal.h"

#include <sys/ioctl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

static struct termios g_orig_term;
static struct termios g_curr_term;
static struct dimensions g_term_size;
static void (*g_resize_handler)(void);

static void term_update_size(int sig) {
    (void) sig;

    struct winsize ws = {0};
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    g_term_size.width = ws.ws_col;
    g_term_size.height = ws.ws_row;

    if (g_resize_handler != NULL) g_resize_handler();
}

static void term_atexit(void) {
    tcsetattr(STDIN_FILENO, TCSANOW, &g_orig_term);
}

void term_init(void) {
    tcgetattr(STDIN_FILENO, &g_orig_term);
    signal(SIGWINCH, term_update_size);

    g_curr_term = g_orig_term;

    atexit(term_atexit);
}

void term_force_update_size(void) {
    term_update_size(67);
}

struct dimensions term_get_size(void) {
    return g_term_size;
}

void term_resize_handler(void (*cb)(void)) {
    g_resize_handler = cb;
}

void term_enable_raw(void) {
    g_curr_term.c_lflag &= ~(ICANON | ECHO);
    g_curr_term.c_cc[VMIN] = 1;
    g_curr_term.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &g_curr_term);
}

void term_disable_raw(void) {
    g_curr_term.c_lflag |= ICANON | ECHO;
    g_curr_term.c_cc[VMIN] = g_orig_term.c_cc[VMIN];
    g_curr_term.c_cc[VTIME] = g_orig_term.c_cc[VTIME];
    tcsetattr(STDIN_FILENO, TCSANOW, &g_curr_term);
}

void term_set_cursor_pos(int line, int col) {
    char seq[64];
    int n = snprintf(seq, sizeof(seq), "\x1b[%d;%dH", line, col);
    write(STDOUT_FILENO, seq, n);
}

void term_clear(void) {
    write(STDOUT_FILENO, "\x1b[2J", 4);
}

void term_move_home(void) {
    write(STDOUT_FILENO, "\x1b[H", 3);
}

void term_hide_cursor(void) {
    write(STDOUT_FILENO, "\x1b[?25l", 6);
}

void term_show_cursor(void) {
    write(STDOUT_FILENO, "\x1b[?25h", 6);
}

void term_fmt_reset(void) {
    write(STDOUT_FILENO, "\x1b[0m", 4);
}

#define FMT_HELPER(enable, t, tl, f, fl) if (enable) write(STDOUT_FILENO, t, tl); else write(STDOUT_FILENO, f, fl);

void term_fmt_bold(bool enable) {
    FMT_HELPER(enable, "\x1b[1m", 4, "\x1b[22m", 5);
}

void term_fmt_faint(bool enable) {
    FMT_HELPER(enable, "\x1b[2m", 4, "\x1b[22m", 5);
}

void term_fmt_italic(bool enable) {
    FMT_HELPER(enable, "\x1b[3m", 4, "\x1b[23m", 5);
}

void term_fmt_underline(bool enable) {
    FMT_HELPER(enable, "\x1b[4m", 4, "\x1b[24m", 5);
}

void term_fmt_blink(bool enable) {
    FMT_HELPER(enable, "\x1b[5m", 4, "\x1b[25m", 5);
}

void term_fmt_reverse(bool enable) {
    FMT_HELPER(enable, "\x1b[7m", 4, "\x1b[27m", 5);
}

void term_fmt_strikethrough(bool enable) {
    FMT_HELPER(enable, "\x1b[9m", 4, "\x1b[29m", 5);
}

void term_fmt_color_fg_reset(void) {
    write(STDOUT_FILENO, "\x1b[39m", 5);
}

void term_fmt_color_bg_reset(void) {
    write(STDOUT_FILENO, "\x1b[49m", 5);
}

void term_fmt_color_fg(struct color c) {
    char seq[64];
    int n = snprintf(seq, sizeof(seq), "\x1b[38;2;%u;%u;%um", (unsigned int) c.r, (unsigned int) c.g, (unsigned int) c.b);
    write(STDOUT_FILENO, seq, n);
}

void term_fmt_color_bg(struct color c) {
    char seq[64];
    int n = snprintf(seq, sizeof(seq), "\x1b[48;2;%u;%u;%um", (unsigned int) c.r, (unsigned int) c.g, (unsigned int) c.b);
    write(STDOUT_FILENO, seq, n);
}

void term_write(char c) {
    term_write_str(&c, 1);
}

void term_write_str(const char* str, int len) {
    write(STDOUT_FILENO, str, len);
}
