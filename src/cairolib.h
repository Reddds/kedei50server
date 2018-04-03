#ifndef CAIROLIB_H
#define CAIROLIB_H

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
 * 125 - radio button for finger*/
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
	CT_FINGER_RADIO = 125
}control_types;

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


struct text_box_data_tag
{
	uint16_t font_size;
	uint8_t r;
	uint8_t g;
	uint8_t b;
	char *text;
};

int cairo_test (cairo_t *cr);
void cairo_line(cairo_t *cr, double stroke_width, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t r, uint8_t g, uint8_t b);
void control_label(cairo_t *cr, uint16_t x, uint16_t y, double size, char *text, uint8_t r, uint8_t g, uint8_t b);
void show_control(cairo_t *cr, dk_control *control);
#endif
