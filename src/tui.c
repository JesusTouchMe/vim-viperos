// Copyright 2025 JesusTouchMe

#include "tui.h"

#include <sys/ioctl.h>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct cell {
    char c;
    bool invert;
};

struct screen {
    struct dimensions size;
    struct cell* prev_cells;
    struct cell* cells;
};

struct dimensions g_terminal_size;
struct screen g_screen;

//NOTE: maybe the terminal related operations should be their own module

static void get_terminal_size(void) {
    struct winsize ws = {0};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0 || ws.ws_row == 0) {
        ws.ws_col = 80;
        ws.ws_row = 24;
    }
    g_terminal_size.width = ws.ws_col;
    g_terminal_size.height = ws.ws_row;
}

static void terminal_clear(void) {
    write(STDOUT_FILENO, "\x1b[2J", 4);
}

static void terminal_move_home(void) {
    write(STDOUT_FILENO, "\x1b[H", 3);
}

static void terminal_hide_cursor(void) {
    write(STDOUT_FILENO, "\x1b[?25l", 6);
}

static void terminal_show_cursor(void) {
    write(STDOUT_FILENO, "\x1b[?25h", 6);
}

static void screen_reallocate(void) {
    free(g_screen.cells);
    free(g_screen.prev_cells);
    size_t sz = g_screen.size.width * g_screen.size.height * sizeof(struct cell);
    g_screen.prev_cells = malloc(sz);
    g_screen.cells = malloc(sz);
    if (g_screen.prev_cells == NULL || g_screen.cells == NULL) exit(1);
    memset(g_screen.prev_cells, 0, sz);
    tui_clear();
}

static void screen_resize(void) {
    get_terminal_size();
    g_screen.size = g_terminal_size;
    screen_reallocate();
}

static void handle_resize(int sig) {
    (void) sig;
    screen_resize();
}

static bool cell_equal(struct cell* a, struct cell* b) {
    return a->c == b->c && a->invert == b->invert;
}

static void draw_cell(int x, int y, struct cell* cell) {
    char seq[64];
    int n = snprintf(seq, sizeof(seq), "\x1b[%d;%dH", y + 1, x + 1);
    write(STDOUT_FILENO, seq, n);

    if (cell->invert) {
        write(STDOUT_FILENO, "\x1b[7m", 4);
        write(STDOUT_FILENO, &cell->c, 1);
        write(STDOUT_FILENO, "\x1b[0m", 4);
    } else {
        write(STDOUT_FILENO, &cell->c, 1);
    }
}

void tui_init(void) {
    terminal_hide_cursor();
    get_terminal_size();
    g_screen.size = g_terminal_size;
    g_screen.prev_cells = NULL;
    g_screen.cells = NULL;
    screen_reallocate();

    signal(SIGWINCH, handle_resize);
}

void tui_destroy(void) {
    terminal_show_cursor();
    terminal_clear();
    free(g_screen.prev_cells);
    free(g_screen.cells);

    write(STDOUT_FILENO, "\n", 1);
}

struct dimensions tui_get_screen_size(void) {
    return g_screen.size;
}

void tui_clear(void) {
    for (int i = 0; i < g_screen.size.width * g_screen.size.height; i++) {
        g_screen.cells[i].c = ' ';
        g_screen.cells[i].invert = false;
    }
}

void tui_put(int x, int y, char c) {
    if (x >= 0 && x < g_screen.size.width && y >= 0 && y < g_screen.size.height)
        g_screen.cells[y * g_screen.size.width + x].c = c;
}

void tui_set_invert(int x, int y, bool invert) {
    if (x >= 0 && x < g_screen.size.width && y >= 0 && y < g_screen.size.height)
        g_screen.cells[y * g_screen.size.width + x].invert = invert;
}

void tui_render(void) {
    terminal_move_home();

    for (int y = 0; y < g_screen.size.height; y++) {
        for (int x = 0; x < g_screen.size.width; x++) {
            int i = y * g_screen.size.width + x;
            if (!cell_equal(&g_screen.cells[i], &g_screen.prev_cells[i])) {
                draw_cell(x, y, &g_screen.cells[i]);
                g_screen.prev_cells[i] = g_screen.cells[i];
            }
        }
    }

    fflush(stdout); // is this needed?
}
