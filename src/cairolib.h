#ifndef CAIROLIB_H
#define CAIROLIB_H

typedef struct hex_color
{
    uint8_t r, g, b;
} hex_color_t;

/*
 * 1 - label within rectangle
 * 2 - text box
 * 
 * 102 - text box for finger
 * 
 * 10 - button
 * 11 - image button
 * 
 * 110 - button for finger
 * 111 - image button for finger
 * 
 * 20 - check box
 * 25 - radio button
 * 
 * 120 - check box for finger
 * 125 - radio button for finger
 *
 *	50 - static image
 *
 * */
typedef enum 
{
	CT_LABEL = 1,
	CT_TEXT_BOX = 2,
	
	CT_FINGER_TEXT_BOX = 102,
	
	
	CT_BUTTON = 10,
	CT_IMAGE_BUTTON = 11,
	
	CT_FINGER_BUTTON = 110,
	CT_FINGER_IMAGE_BUTTON = 111,
	
	
	CT_CHECK_BOX = 20,
	CT_RADIO = 25,
	
	CT_FINGER_CHECK_BOX = 120,
	CT_FINGER_RADIO = 125,

	CT_STATIC_IMAGE = 50
}control_types;

typedef enum
{
	COL_BLACK,
	COL_BG_COLOR,
	COL_HI_COLOR_1,
	COL_HI_COLOR_2,
	COL_LO_COLOR_1,
	COL_LO_COLOR_2
}std_colors_t;

typedef struct 
{
	uint16_t id;
	control_types type;
	uint16_t left;
	uint16_t top;
	uint16_t right;
	uint16_t bottom;
	void *control_data;
}dk_control;


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
	uint32_t image_len;
	uint8_t *image_data;
};

hex_color_t *get_std_color(std_colors_t cid);
void cairo_clear_all(cairo_t *cr);
int cairo_test (cairo_t *cr);
void cairo_line(cairo_t *cr, double stroke_width, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, hex_color_t color);
void control_label(cairo_t *cr, uint16_t x, uint16_t y, double size, char *text, hex_color_t color);
void show_control(cairo_t *cr, dk_control *control);
void set_text(cairo_t *cr, dk_control *control, char *text);
void draw_text_in_rect(cairo_t *cr, uint16_t font_size, uint16_t left, uint16_t top, uint16_t width, uint16_t height,
						hex_color_t *color,
						hex_color_t *bg_color,
						char *text);
#endif
