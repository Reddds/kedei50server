#ifndef CAIROLIB_H
#define CAIROLIB_H

#include "dk_controls.h"

typedef struct hex_color
{
    uint8_t r, g, b;
} hex_color_t;



typedef enum
{
	COL_BLACK,
	COL_BG_COLOR,
	COL_HI_COLOR_1,
	COL_HI_COLOR_2,
	COL_LO_COLOR_1,
	COL_LO_COLOR_2
}std_colors_t;

struct panel_data_tag
{
	hex_color_t bg_color;
};


struct label_data_tag
{
	uint16_t font_size;
	hex_color_t color;
	char *text;
};

struct text_box_data_tag
{
	uint16_t font_size;
	hex_color_t color;
	char *text;
};

struct dk_image_data_tag
{
	uint8_t image_type;
	uint8_t scale_type;
	hex_color_t bg_color;
	uint32_t image_len;
	uint8_t *image_data;
};

hex_color_t *get_std_color(std_colors_t cid);
void cairo_clear_all(cairo_t *cr);
int cairo_test (cairo_t *cr);
void cairo_line(cairo_t *cr, double stroke_width, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, hex_color_t color);
void control_label(cairo_t *cr, uint16_t x, uint16_t y, double size, char *text, hex_color_t color);
control_position_t show_control(cairo_t *cr, dk_control *control);
control_position_t set_text(cairo_t *cr, dk_control *control, char *text);
void draw_text_in_rect(cairo_t *cr, uint16_t font_size, uint16_t left, uint16_t top, uint16_t width, uint16_t height,
						hex_color_t *color,
						hex_color_t *bg_color,
						char *text);
#endif
