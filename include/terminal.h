// Copyright 2025 JesusTouchMe

#ifndef TERMINAL_H
#define TERMINAL_H 1

#include <stdbool.h>

// https://github.com/raysan5/raylib/blob/master/src/raylib.h#L147
#if defined(__cplusplus)
    #define CLITERAL(type)      type
#else
    #define CLITERAL(type)      (type)
#endif

struct color {
    unsigned char r, g, b;
};

struct dimensions {
    int width;
    int height;
};

void term_init(void); // initialize internal state and shit

void term_force_update_size(void);
struct dimensions term_get_size(void);

void term_resize_handler(void (*cb)(void));

void term_enable_raw(void);
void term_disable_raw(void);

void term_set_cursor_pos(int line, int col);

void term_clear(void);
void term_move_home(void);

void term_hide_cursor(void);
void term_show_cursor(void);

// format

void term_fmt_reset(void);

void term_fmt_bold(bool enable);
void term_fmt_faint(bool enable);
void term_fmt_italic(bool enable);
void term_fmt_underline(bool enable);
void term_fmt_blink(bool enable);
void term_fmt_reverse(bool enable);
void term_fmt_strikethrough(bool enable);

void term_fmt_color_fg_reset(void);
void term_fmt_color_bg_reset(void);
void term_fmt_color_fg(struct color c);
void term_fmt_color_bg(struct color c);

// write

void term_write(char c);
void term_write_str(const char* str, int len);

#endif //TERMINAL_H