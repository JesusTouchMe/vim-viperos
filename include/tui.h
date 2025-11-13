// Copyright 2025 JesusTouchMe

#ifndef TUI_H
#define TUI_H 1

#include <stdbool.h>

struct dimensions {
    int width;
    int height;
};

void tui_init(void);
void tui_destroy(void);

struct dimensions tui_get_screen_size(void);

void tui_clear(void);

void tui_put(int x, int y, char c);
void tui_set_invert(int x, int y, bool invert);

void tui_render(void);

#endif //TUI_H