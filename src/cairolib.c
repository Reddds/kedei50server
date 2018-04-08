#include <stdio.h>
#include <cairo.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "kedei_lcd_v50_pi_pigpio.h"
#include "cairolib.h"






//hex_color_t BG_COLOR =  { 0xd4, 0xd0, 0xc8 };
//hex_color_t HI_COLOR_1 = { 0xff, 0xff, 0xff };
//hex_color_t HI_COLOR_2 = { 0xd4, 0xd0, 0xc8 };
//hex_color_t LO_COLOR_1 = { 0x80, 0x80, 0x80 };
//hex_color_t LO_COLOR_2 = { 0x40, 0x40, 0x40 };
//hex_color_t BLACK  = { 0, 0, 0 };

hex_color_t std_colors[] =
{
	{ 0, 0, 0 },
	{ 0xd4, 0xd0, 0xc8 },
	{ 0xff, 0xff, 0xff },
	{ 0xd4, 0xd0, 0xc8 },
	{ 0x80, 0x80, 0x80 },
	{ 0x40, 0x40, 0x40 }
};

hex_color_t *get_std_color(std_colors_t cid)
{
	return &std_colors[cid];
}

static void
set_hex_color (cairo_t *cr, std_colors_t color)
{
	hex_color_t *col = get_std_color(color);
	//printf("Set color r = %u g = %u b = %u \n", col->r, col->g, col->b);
    cairo_set_source_rgb (cr,
			 col->r / 255.0,
			 col->g / 255.0,
			 col->b / 255.0);
}

static void
bevel_box (cairo_t *cr, int x, int y, int width, int height)
{
    cairo_save (cr);

    cairo_set_line_width (cr, 1.0);
    cairo_set_line_cap (cr, CAIRO_LINE_CAP_SQUARE);

    /* Fill and highlight */
    set_hex_color (cr, COL_HI_COLOR_1);
    cairo_rectangle (cr, x, y, width, height);
    cairo_fill (cr);

    /* 2nd highlight */
    set_hex_color (cr, COL_HI_COLOR_2);
    cairo_move_to (cr, x + 1.5, y + height - 1.5);
    cairo_rel_line_to (cr, width - 3, 0);
    cairo_rel_line_to (cr, 0, - (height - 3));
    cairo_stroke (cr);

    /* 1st lowlight */
    set_hex_color (cr, COL_LO_COLOR_1);
    cairo_move_to (cr, x + .5, y + height - 1.5);
    cairo_rel_line_to (cr, 0, - (height - 2));
    cairo_rel_line_to (cr, width - 2, 0);
    cairo_stroke (cr);

    /* 2nd lowlight */
    set_hex_color (cr, COL_LO_COLOR_2);
    cairo_move_to (cr, x + 1.5, y + height - 2.5);
    cairo_rel_line_to (cr, 0, - (height - 4));
    cairo_rel_line_to (cr, width - 4, 0);
    cairo_stroke (cr);

    cairo_restore (cr);
}

static void
bevel_circle (cairo_t *cr, int x, int y, int width)
{
    double radius = (width-1)/2.0 - 0.5;

    cairo_save (cr);

    cairo_set_line_width (cr, 1);

    /* Fill and Highlight */
    set_hex_color (cr, COL_HI_COLOR_1);
    cairo_arc (cr, x+radius+1.5, y+radius+1.5, radius,
	       0, 2* M_PI);
    cairo_fill (cr);

    /* 2nd highlight */
    set_hex_color (cr, COL_HI_COLOR_2);
    cairo_arc (cr, x+radius+0.5, y+radius+0.5, radius,
	       0, 2 * M_PI);
    cairo_stroke (cr);

    /* 1st lowlight */
    set_hex_color (cr, COL_LO_COLOR_1);
    cairo_arc (cr, x+radius+0.5, y+radius+0.5, radius,
	       3 * M_PI_4, 7 * M_PI_4);
    cairo_stroke (cr);

    /* 2nd lowlight */
    set_hex_color (cr, COL_LO_COLOR_2);
    cairo_arc (cr, x+radius+1.5, y+radius+1.5, radius,
	       3 * M_PI_4, 7 * M_PI_4);
    cairo_stroke (cr);

    cairo_restore (cr);
}

/* Slightly smaller than specified to match interior size of bevel_box */
static void
flat_box (cairo_t *cr, int x, int y, int width, int height)
{
    cairo_save (cr);

    /* Fill background */
    set_hex_color (cr, COL_HI_COLOR_1);
    cairo_rectangle (cr, x+1, y+1, width-2, height-2);
    cairo_fill (cr);

    /* Stroke outline */
    cairo_set_line_width (cr, 1.0);
    set_hex_color (cr, COL_BLACK);
    cairo_rectangle (cr, x + 1.5, y + 1.5, width - 3, height - 3);
    cairo_stroke (cr);

    cairo_restore (cr);
}

static void
flat_circle (cairo_t *cr, int x, int y, int width)
{
    double radius = (width - 1) / 2.0;

    cairo_save (cr);

    /* Fill background */
    set_hex_color (cr, COL_HI_COLOR_1);
    cairo_arc (cr, x+radius+0.5, y+radius+0.5, radius-1,
	       0, 2 * M_PI);
    cairo_fill (cr);

    /* Fill background */
    cairo_set_line_width (cr, 1.0);
    set_hex_color (cr, COL_BLACK);
    cairo_arc (cr, x+radius+0.5, y+radius+0.5, radius-1,
	       0, 2 * M_PI);
    cairo_stroke (cr);

    cairo_restore (cr);
}

static void
groovy_box (cairo_t *cr, int x, int y, int width, int height)
{
    cairo_save (cr);

    /* Highlight */
    set_hex_color (cr, COL_HI_COLOR_1);
    cairo_set_line_width (cr, 2);
    cairo_rectangle (cr, x + 1, y + 1, width - 2, height - 2);
    cairo_stroke (cr);

    /* Lowlight */
    set_hex_color (cr, COL_LO_COLOR_1);
    cairo_set_line_width (cr, 1);
    cairo_rectangle (cr, x + 0.5, y + 0.5, width - 2, height - 2);
    cairo_stroke (cr);

    cairo_restore (cr);
}

#define CHECK_BOX_SIZE 13
#define CHECK_COLOR COL_BLACK

typedef enum {UNCHECKED, CHECKED} checked_status_t;

static void
check_box (cairo_t *cr, int x, int y, checked_status_t checked)
{
    cairo_save (cr);

    bevel_box (cr, x, y, CHECK_BOX_SIZE, CHECK_BOX_SIZE);

    if (checked) {
	set_hex_color (cr, CHECK_COLOR);
	cairo_move_to (cr, x + 3, y + 5);
	cairo_rel_line_to (cr, 2.5, 2);
	cairo_rel_line_to (cr, 4.5, -4);
	cairo_rel_line_to (cr, 0, 3);
	cairo_rel_line_to (cr, -4.5, 4);
	cairo_rel_line_to (cr, -2.5, -2);
	cairo_close_path (cr);
	cairo_fill (cr);
    }

    cairo_restore (cr);
}

#define RADIO_SIZE CHECK_BOX_SIZE
#define RADIO_DOT_COLOR COL_BLACK
static void
radio_button (cairo_t *cr, int x, int y, checked_status_t checked)
{

    cairo_save (cr);

    bevel_circle (cr, x, y, RADIO_SIZE);

    if (checked) {
	set_hex_color (cr, RADIO_DOT_COLOR);
	cairo_arc (cr,
		   x + (RADIO_SIZE-1) / 2.0 + 0.5,
		   y + (RADIO_SIZE-1) / 2.0 + 0.5,
		   (RADIO_SIZE-1) / 2.0 - 3.5,
		   0, 2 * M_PI);
	cairo_fill (cr);
    }

    cairo_restore (cr);
}

static void
draw_bevels (cairo_t *cr, int width, int height)
{
    int check_room = (width - 20) / 3;
    int check_pad = (check_room - CHECK_BOX_SIZE) / 2;

    groovy_box (cr, 5, 5, width - 10, height - 10);

    check_box (cr, 10+check_pad, 10+check_pad, UNCHECKED);
    check_box (cr, check_room+10+check_pad, 10+check_pad, CHECKED);
    flat_box (cr, 2 * check_room+10+check_pad, 10+check_pad,
	      CHECK_BOX_SIZE, CHECK_BOX_SIZE);

    radio_button (cr, 10+check_pad, check_room+10+check_pad, UNCHECKED);
    radio_button (cr, check_room+10+check_pad, check_room+10+check_pad, CHECKED);
    flat_circle (cr, 2 * check_room+10+check_pad, check_room+10+check_pad, CHECK_BOX_SIZE);
}

int cairo_test (cairo_t *cr)
{
	printf("Cairo start\n");
    /*cairo_t *cr;
    cairo_surface_t *surface;

    surface = cairo_image_surface_create_for_data (image, CAIRO_FORMAT_RGB24,
						   WIDTH, HEIGHT, STRIDE);
    cr = cairo_create (surface);
*/
    cairo_rectangle (cr, 0, 0, LCD_WIDTH, LCD_HEIGHT);
    set_hex_color (cr, COL_BG_COLOR);
    cairo_fill (cr);

	
    draw_bevels (cr, LCD_WIDTH, LCD_HEIGHT);
    /*set_hex_color (cr, BLACK);
    cairo_move_to (cr, 100, 100);
	cairo_line_to (cr, 300, 300);
	cairo_stroke (cr);*/


	//printf("Cairo save image to bevels.png");
    //cairo_surface_write_to_png (surface, "bevels.png");

    //cairo_destroy (cr);

    //cairo_surface_destroy (surface);

    return 0;
}

void cairo_line(cairo_t *cr, double stroke_width, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, hex_color_t color)
{
	cairo_set_source_rgb (cr, color.r / 255.0, color.g / 255.0, color.b / 255.0);
    cairo_move_to (cr, x1, y1);
	cairo_line_to (cr, x2, y2);
	cairo_set_line_width(cr, stroke_width);
	cairo_stroke (cr);
}

void control_label(cairo_t *cr, uint16_t x, uint16_t y, double size, char *text, hex_color_t color)
{
	cairo_set_source_rgb (cr, color.r / 255.0, color.g / 255.0, color.b / 255.0);
	cairo_select_font_face (cr, "sans", 0, 0);
	cairo_set_font_size (cr, size);
	cairo_move_to (cr, x, y);
    cairo_show_text (cr, text);
	cairo_set_line_width (cr, 1.0);
    cairo_stroke (cr);
}

void draw_text_in_rect(cairo_t *cr, uint16_t font_size, uint16_t left, uint16_t top, uint16_t width, uint16_t height,
						hex_color_t *color,
						hex_color_t *bg_color,
						char *text)
{
	if(text == NULL)
		return;

	cairo_text_extents_t extents;
		
	cairo_save (cr);

	cairo_rectangle (cr, left, top, width, height);
	cairo_clip (cr);

	cairo_rectangle (cr, left, top, width, height);
	cairo_set_source_rgb (cr, 
			bg_color->r / 255.0, 
			bg_color->g / 255.0, 
			bg_color->b / 255.0);

	cairo_stroke_preserve(cr);
	cairo_fill(cr);
	
	cairo_set_source_rgb (cr, 
			color->r / 255.0, 
			color->g / 255.0, 
			color->b / 255.0);
	cairo_select_font_face (cr, "sans", 0, 0);
	cairo_set_font_size (cr, font_size);
	cairo_text_extents (cr, "Wygitf", &extents);

	//printf("y_bearing = %f, height = %f\n", extents.y_bearing, extents.height);

	double bot_ext = extents.height + extents.y_bearing;
	double full_height = extents.height + bot_ext;
	uint16_t top_pos = top + height - bot_ext;
	if(full_height < height)
	{
		top_pos -= height / 2.0 - (full_height / 2);// + extents.y_bearing
	}
	
	cairo_move_to (cr, left, top_pos);
	cairo_show_text (cr, text);
	cairo_set_line_width (cr, 1.0);
	cairo_stroke (cr);

	cairo_restore (cr);
}

control_position_t draw_dk_panel(cairo_t *cr, dk_control *control)
{
	cairo_save (cr);

	control_position_t abs_pos = get_abs_control_pos(control);

	cairo_rectangle (cr, abs_pos.left, abs_pos.top, abs_pos.width, abs_pos.height);

	struct panel_data_tag *panel_data = ((struct panel_data_tag *)control->control_data);
	cairo_set_source_rgb (cr, 
			panel_data->bg_color.r / 255.0, 
			panel_data->bg_color.g / 255.0, 
			panel_data->bg_color.b / 255.0);

	cairo_stroke_preserve(cr);
	cairo_fill(cr);
	
	
    cairo_restore (cr);
    return abs_pos;
}

control_position_t draw_dk_label(cairo_t *cr, dk_control *control)
{
	//cairo_text_extents_t extents;
	cairo_save (cr);
	
	//printf("cairo draw_label left = %u, top = %u, right = %u, bottom = %u\n",
	//	control->left,  control->top, control->right, control->bottom);
	control_position_t abs_pos = get_abs_control_pos(control);
	printf("Abs Pos: left = %u, top = %u, width = %u, height = %u\n",
		abs_pos.left, abs_pos.top, abs_pos.width, abs_pos.height);

	hex_color_t *bg_color = get_std_color(COL_BG_COLOR);
	// Determine bg color
	if(control->parent_id != 0)
	{
		dk_control *parent_control = find_control(control->parent_id);
		if(parent_control != NULL)
		{
			cairo_rectangle (cr, parent_control->left, parent_control->top, parent_control->width, parent_control->height);
			cairo_clip (cr);
			
			if(parent_control->type == CT_PANEL)
			{
				bg_color = &((struct panel_data_tag *)parent_control->control_data)->bg_color;
				printf("Determined bg color: r = %u, g = %u, b = %u\n",
					bg_color->r, bg_color->g, bg_color->b);
			}
		}
		else
		{
			printf("Error find parent control!\n");
		}
	}

	draw_text_in_rect(cr, ((struct label_data_tag *)control->control_data)->font_size,
				abs_pos.left, abs_pos.top,
                 abs_pos.width, abs_pos.height,
                 &((struct label_data_tag *)control->control_data)->color,
                 bg_color,
                 ((struct label_data_tag *)control->control_data)->text);
		
    cairo_restore (cr);
    return abs_pos;
}

control_position_t draw_dk_text_box(cairo_t *cr, dk_control *control)
{
	//cairo_text_extents_t extents;
	cairo_save (cr);
	
	printf("cairo draw_text_box left = %u, top = %u, width = %u, height = %u\n",
		control->left,  control->top, control->width, control->height);

	control_position_t abs_pos = get_abs_control_pos(control);
	printf("Abs Pos: left = %u, top = %u, width = %u, height = %u\n",
		abs_pos.left, abs_pos.top, abs_pos.width, abs_pos.height);
	bevel_box (cr, abs_pos.left, abs_pos.top, abs_pos.width, abs_pos.height);
	
	draw_text_in_rect(cr, ((struct text_box_data_tag *)control->control_data)->font_size,
				abs_pos.left + 3, abs_pos.top + 3,
                 abs_pos.width - 6, abs_pos.height - 6,
                 &((struct text_box_data_tag *)control->control_data)->color,
                 get_std_color(COL_HI_COLOR_1),
                 ((struct text_box_data_tag *)control->control_data)->text);
		
    cairo_restore (cr);
    return abs_pos;
}

typedef struct
{
    uint8_t *data;
    uint32_t max_size;
    uint32_t pos;
} png_stream_to_byte_array_closure_t;

static cairo_status_t read_png_stream_from_byte_array (void *in_closure, uint8_t *data, unsigned int length)
{
    png_stream_to_byte_array_closure_t *closure =
		(png_stream_to_byte_array_closure_t *) in_closure;

    //log_message(STORE_LOGLVL_DEBUG, "ro_composite_tile: reading from byte array: pos: %i, length: %i", closure->pos, length);

    if ((closure->pos + length) > (closure->max_size))
        return CAIRO_STATUS_READ_ERROR;

    memcpy (data, (closure->data + closure->pos), length);
    closure->pos += length;

    return CAIRO_STATUS_SUCCESS;
}

control_position_t draw_dk_image(cairo_t *cr, dk_control *control)
{
	//cairo_text_extents_t extents;
	cairo_save (cr);
	
	//printf("cairo draw_dk_image left = %u, top = %u, right = %u, bottom = %u\n",
	//	control->left,  control->top, control->right, control->bottom);
	control_position_t abs_pos = get_abs_control_pos(control);

	struct dk_image_data_tag *dk_image_data = control->control_data;
	
	png_stream_to_byte_array_closure_t closure;
	closure.data = dk_image_data->image_data;
    closure.pos = 0;
    closure.max_size = dk_image_data->image_len;

    //printf("Closure: pos = %u, max_size = %u\n",
	//	closure.pos, closure.max_size);

	//printf("Control width: %u, height: %u\n", width, height);
	
	//cairo_rectangle (cr, control->left, control->top, width, height);
	cairo_translate(cr, abs_pos.left, abs_pos.top);
	cairo_rectangle (cr, 0, 0, abs_pos.width, abs_pos.height);
	cairo_clip (cr);
	cairo_new_path (cr); /* path not consumed by clip()*/

	cairo_surface_t *image = cairo_image_surface_create_from_png_stream(&read_png_stream_from_byte_array, &closure);
	double img_width = cairo_image_surface_get_width(image);
	double img_height = cairo_image_surface_get_height(image);

	//printf("Image width = %f, height = %f\n", img_width, img_height);

	double scale_x = 1;// width / img_width;
	double scale_y = 1;// height / img_height;

	double off_x = 0;
	double off_y = 0;
	switch(dk_image_data->scale_type)
	{
		case 1: //1 - fit width
			scale_y = scale_x = abs_pos.width / img_width;
			break;
		case 2:// - fit height
			scale_y = scale_x = abs_pos.height / img_height;
			break;
		case 3:// - fit on max dimension
			scale_x = abs_pos.width / img_width;
			scale_y = abs_pos.height / img_height;
			if(scale_x > scale_y)
				scale_y = scale_x;
			else
				scale_x = scale_y;
			break;
		case 4:// - fit on min dimension
			scale_x = abs_pos.width / img_width;
			scale_y = abs_pos.height / img_height;
			if(scale_x < scale_y)
				scale_y = scale_x;
			else
				scale_x = scale_y;
			break;
		case 5:// - stretch
			scale_x = abs_pos.width / img_width;
			scale_y = abs_pos.height / img_height;
			break;
	}

	
	off_x = -(img_width * scale_x - abs_pos.width) / 2 / scale_x;
	off_y = -(img_height * scale_y - abs_pos.height) / 2 / scale_y;

	//printf("Image scale_type = %u, scale_x = %f, scale_y = %f, off_x = %f, off_y = %f\n", dk_image_data->scale_type, scale_x, scale_y, off_x, off_y);
	cairo_scale (cr, scale_x, scale_y);
	cairo_set_source_surface (cr, image, off_x, off_y);
	cairo_paint (cr);
	cairo_surface_destroy(image);	
    cairo_restore (cr);
    return abs_pos;
}

void cairo_clear_all(cairo_t *cr)
{
    cairo_rectangle (cr, 0, 0, LCD_WIDTH, LCD_HEIGHT);
    set_hex_color (cr, COL_BG_COLOR);
    cairo_fill (cr);
	
}

control_position_t show_control(cairo_t *cr, dk_control *control)
{
	switch(control->type)
	{
		case CT_LABEL:
			return draw_dk_label(cr, control);
			break;
		case CT_TEXT_BOX:
			return draw_dk_text_box(cr, control);
			break;
	
		case CT_FINGER_TEXT_BOX:
			break;
	
	
		case CT_BUTTON:
			break;
		case CT_IMAGE_BUTTON:
			break;
	
		case CT_FINGER_BUTTON:
			break;
		case CT_FINGER_IMAGE_BUTTON:
			break;
	
	
		case CT_CHECK_BOX:
			break;
		case CT_RADIO:
			break;
	
		case CT_FINGER_CHECK_BOX:
			break;
		case CT_FINGER_RADIO:
			break;

		case CT_STATIC_IMAGE:
			return draw_dk_image(cr, control);
			break;
		case CT_PANEL:
			return draw_dk_panel(cr, control);
			break;
	}
	return UNDEF_CONTROL_POS;
}

bool change_text(char **old, char *new)
{
	//printf("change_text %s => %s\n", *old, new);
	uint16_t slen = 0;
	if(new != NULL)
		slen = strlen(new);
	if(*old == NULL)
	{
		if(slen > 0)
		{
			*old = malloc(slen);
		}
		else
		{
			return false;
		}
	}
	else
	{
		if(slen > 0)
		{
			if(slen != strlen(*old))
			{
				//printf("Before realloc\n");
				*old = (char *)realloc(*old, slen);
				//printf("After realloc\n");
			}
		}
		else
		{
			*old = NULL;
		}
	}
	if(*old != NULL)
	{
		strcpy(*old, new);
		//printf("Copied!\n");
	}
	else
	{
		printf("*old = NULL!!\n");
	}
	
	return true;
}

control_position_t label_set_text(cairo_t *cr, dk_control *control, char *text)
{
	//printf("label_set_text\n");
	if(change_text(&((struct label_data_tag *)control->control_data)->text, text))
	{
		//printf("draw_dk_labeldraw_dk_label new text = %s\n", ((struct label_data_tag *)control->control_data)->text);
		return draw_dk_label(cr, control);
	}
	else
	{
		printf("Error changing text!\n");
	}
	return UNDEF_CONTROL_POS;
}

control_position_t text_box_set_text(cairo_t *cr, dk_control *control, char *text)
{
	if(change_text(&((struct text_box_data_tag *)control->control_data)->text, text))
	{
		return draw_dk_text_box(cr, control);
	}
	return UNDEF_CONTROL_POS;
}

control_position_t set_text(cairo_t *cr, dk_control *control, char *text)
{
		switch(control->type)
	{
		case CT_LABEL:
			return label_set_text(cr, control, text);
			break;
		case CT_TEXT_BOX:
			return text_box_set_text(cr, control, text);
			break;
	
		case CT_FINGER_TEXT_BOX:
			break;
	
	
		case CT_BUTTON:
			break;
		case CT_IMAGE_BUTTON:
			break;
	
		case CT_FINGER_BUTTON:
			break;
		case CT_FINGER_IMAGE_BUTTON:
			break;
	
	
		case CT_CHECK_BOX:
			break;
		case CT_RADIO:
			break;
	
		case CT_FINGER_CHECK_BOX:
			break;
		case CT_FINGER_RADIO:
			break;
		case CT_STATIC_IMAGE:
			break;
		case CT_PANEL:
			break;
	}
	return UNDEF_CONTROL_POS;

}
