// Copyright 2025 JesusTouchMe

#include "tui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct cell {
    char c;
    bool invert;
};

struct screen {
    struct dimensions size;
    struct cell* prev_cells;
    struct cell* cells;
};

struct screen g_screen;

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
    g_screen.size = term_get_size();
    screen_reallocate();
}

static bool cell_equal(struct cell* a, struct cell* b) {
    return a->c == b->c && a->invert == b->invert;
}

static void draw_cell(int x, int y, struct cell* cell) {
    term_set_cursor_pos(y + 1, x + 1);

    if (cell->invert) {
        term_fmt_reverse(true);
        term_write(cell->c);
        term_fmt_reverse(false);
    } else {
        term_write(cell->c);
    }
}

void tui_init(void) {
    term_hide_cursor();
    term_force_update_size();
    g_screen.size = term_get_size();
    g_screen.prev_cells = NULL;
    g_screen.cells = NULL;
    screen_reallocate();

    term_resize_handler(screen_resize);
}

void tui_destroy(void) {
    term_show_cursor();
    term_clear();
    free(g_screen.prev_cells);
    free(g_screen.cells);

    term_write('\n');
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
    term_move_home();

    for (int y = 0; y < g_screen.size.height; y++) {
        for (int x = 0; x < g_screen.size.width; x++) {
            int i = y * g_screen.size.width + x;
            if (!cell_equal(&g_screen.cells[i], &g_screen.prev_cells[i])) {
                draw_cell(x, y, &g_screen.cells[i]);
                g_screen.prev_cells[i] = g_screen.cells[i];
            }
        }
    }
}
