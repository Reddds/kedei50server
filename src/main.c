/*
 * main.c
 * 
 * Copyright 2018  <pi@raspberrypi>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */


#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <ctype.h>
#include <time.h>
#include <png.h>
#include <inttypes.h>
#include <pthread.h>
#include <time.h>
#include <pwd.h>
#include <libconfig.h>

//cairo
#include <cairo.h>
#include <math.h>
#include "cairolib.h"


#include "kedei_lcd_v50_pi_pigpio.h"

#define SETTING_FILE "/.config/kedeilcd"

#define MYBUF 4000
// Size of signature
#define SIGNATURE_SIZE 4
// 24bit colors for speed
// bytes in one line
//#define SCREEN_BUF_LINE_LEN LCD_WIDTH * 4
#define STRIDE (LCD_WIDTH * 4)
#define SCREEN_BUFFER_LEN (LCD_HEIGHT * STRIDE)
#define READ_TIMEOUT_US 200

config_t cfg; 
config_setting_t *root, *setting, *group;//, *array;

int client_to_server;
char input_buf[MYBUF];
//unsigned char image[STRIDE * LCD_HEIGHT];
uint8_t screen_buffer[SCREEN_BUFFER_LEN];

const char acOpen[]  = {"\"[<{"};
const char acClose[] = {"\"]>}"};

extern volatile int touch_raw_x;
extern volatile int touch_raw_y;
extern volatile uint16_t touch_x, touch_y;

#define MAX_CONTROL 256
/*
 * Types:
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
 * */
dk_control dk_controls[MAX_CONTROL];

int last_control = -1;

cairo_t *cr;
cairo_surface_t *surface;

pthread_t tid[2];
void* doTimeShow(void *arg);
pthread_mutex_t lock_draw;
bool touch_calibrated = false;
extern volatile int touch_offset_x, touch_offset_y;
extern volatile double touch_scale_x, touch_scale_y;

bool compare_signature(uint8_t sig[], char* val)
{
	int len = strlen(val);
	if(len > SIGNATURE_SIZE)
	{
		printf("Error! compare signature with wrong len! Need: %d, found: %s (%d)\n", SIGNATURE_SIZE, val, len);
		return false;
	}
	for(uint8_t i = 0; i < len; i++)
	{
		if(sig[i] != val[i])
			return false;
	}
	return true;
}

// read with timeout
// read_count - real read
bool my_read_count(int fdesc, void *buf, int count, int *read_count)
{
	*read_count = 0;
	struct timespec gettime_now;
	long int start_time;
	long int time_difference;
	clock_gettime(CLOCK_REALTIME, &gettime_now);
	start_time = gettime_now.tv_nsec;
	while(*read_count < count)
	{
		int cur_read = read(fdesc, buf, count - *read_count);
		if(cur_read < 0)
		{
			perror("read");
			return false;
		}
		*read_count += cur_read;
		if(*read_count != count)
		{
			clock_gettime(CLOCK_REALTIME, &gettime_now);
			time_difference = gettime_now.tv_nsec - start_time;
			if (time_difference < 0)
				time_difference += 1000000000;				//(Rolls over every 1 second)
			if (time_difference > (READ_TIMEOUT_US * 1000))		//Delay for # nS
				return false;
		}
	}
	
	return true;
}

bool my_read(int fdesc, void *buf, int count)
{
	int rcnt;
	return my_read_count(fdesc, buf, count, &rcnt);
}

bool skip_read(int skip)
{
	uint8_t buf[128];
	int skipped = 0;
	while(skipped < skip)
	{
		int cur_skip = skip - skipped;
		if(cur_skip > 128)
		{
			cur_skip = 128;
		}
		//printf("\nSkippppp %d\n", cur_skip);
		bool is_readed = my_read(client_to_server, buf, cur_skip);
		if(!is_readed)
		{
			printf("Error in skipping!\n");
			return false;
		}
		skipped += cur_skip;
	}
	return true;
}

void show_part(uint16_t left, uint16_t top, uint16_t width, uint16_t height)
{
	//printf("Swow part left = %u, top = %u, width = %u, height = %u\n\n",
	//	left, top, width, height);
	if(left + width > LCD_WIDTH || top + height > LCD_HEIGHT)
	{
		printf("Error show part! Wrong size!\n");
		return;
	}
	 
	 
	 
	/*printf("First line: \n");
	for(int i = 0; i < SCREEN_BUF_LINE_LEN; i++)
	{
		printf("%02X ", screen_buffer[i]);
	}
	printf("\n");*/
	lcd_setframe(left, top, width, height);
//	uint16_t rowbytes = (width * 3);

	for(uint16_t p = 0; p < height; p++) {
		// p = relative page address (y)
		int cur_line_start = (top + p) * STRIDE;
		for (uint16_t c = 0; c < width; c++) {
			int cur_pos = cur_line_start + (left + c) * 4;
			// c = relative column address (x)
			//fread(buf, 3, 1, f);

			// B buf[0]
			// G buf[1]
			// R buf[2]
			// 18bit color mode
			lcd_colorRGB(screen_buffer[cur_pos + 2], screen_buffer[cur_pos + 1], screen_buffer[cur_pos]);
		}
	}
}

void receive_bmp_file(uint8_t byte2, uint8_t byte3)
{
	/*FILE * fp;
	fp = fopen ("/tmp/receive.bmp", "wb");
	if (! fp) 
	{
		perror("open");
        return;
    }*/
	
	
	uint8_t buf[32];
	uint32_t isize = 0, ioffset, iwidth, iheight, ibpp, rowbytes;
	uint16_t show_width, show_height;
	buf[2] = byte2;
	buf[3] = byte3;
	uint8_t rrr = sizeof(buf) - 4;
	bool is_readed = my_read(client_to_server, &buf[4], rrr);
	if(!is_readed)
	{
		printf("Error reading bmp!\n");
		return;
	}
	
	isize =	 	READ_32(buf, 2);
	ioffset = 	READ_32(buf, 0x0A);
	iwidth =	READ_32(buf, 0x12);
	iheight = 	READ_32(buf, 0x16);
	ibpp =		READ_16(buf, 0x1C);
	printf("\n\n");
	printf("File Size: %u\nOffset: %u\nWidth: %u\nHeight: %u\nBPP: %u\n\n",isize,ioffset,iwidth,iheight,ibpp);
	
	uint16_t skip_bytes = ioffset - sizeof(buf);
	//printf("Skip %u bites before start offset\n", skip_bytes);
	if(!skip_read(skip_bytes))		
	{
		printf("Error reading BMP skip!\n");
		return;
	}
	
	show_width = iwidth;
	if(show_width > LCD_WIDTH)
		show_width = LCD_WIDTH;
		
	show_height = iheight;
	if(show_height > LCD_HEIGHT)
		show_height = LCD_HEIGHT;
	
	uint8_t d = (iwidth * 3) % 4;
	rowbytes = (iwidth * 3);
	// needed bytes in line
	//int read_line_len = rowbytes;
	if(d > 0)
	{
		rowbytes += 4 - d;
	}
	
	int start_bottom_row = LCD_HEIGHT - 1;
	if(iheight > LCD_HEIGHT)
	{
		// Skip bottom lines
		for(uint16_t p = 0; p < iheight - LCD_HEIGHT; p++)
		{
			if(!skip_read(rowbytes))		
			{
				printf("Error reading BMP!\n");
				return;
			}
		} 
		iheight = LCD_HEIGHT;
	}
	
	if(iheight < LCD_HEIGHT)
	{
		start_bottom_row -= LCD_HEIGHT - iheight;
	}
	
	int pos_in_local_buf;
	// skip not needed bytes in line
	int tail = rowbytes - LCD_WIDTH * 3;
	// if rowbytes < SCREEN_BUF_LINE_LEN
	if(tail < 0)
	{
		tail = d;
	}
	else
	{
		//read_line_len = LCD_WIDTH * 3;
		
	}
	printf("tile = %d, startPos = %d, start_bottom_row = %d", tail, start_bottom_row * STRIDE, start_bottom_row);
	//printf("read_line_len = %d, rowbytes = %lu\n", read_line_len, rowbytes);
	//printf("reading line: ");
	for (uint16_t p = 0; p < iheight; p++) 
	{
		pos_in_local_buf = (start_bottom_row - p) * STRIDE;
		//printf("l: %u (pos: %d), ", p, pos_in_local_buf);
				
		
		/*is_readed = my_read(client_to_server, &screen_buffer[pos_in_local_buf], read_line_len);
		if(!is_readed)
		{
			printf("\nError reading BMP 11!\n");

			return;
		}*/
		//fwrite(&screen_buffer[pos_in_local_buf], 1, read_line_len, fp);
		
		
		
		
		//screen_buffer[pos_in_local_buf]  		= screen_buffer[pos_in_local_buf + 3] = 0;
		//screen_buffer[pos_in_local_buf + 1] 	= screen_buffer[pos_in_local_buf + 4] = 0;
		//screen_buffer[pos_in_local_buf + 2] 	= screen_buffer[pos_in_local_buf + 5] = 0xff;
		
		// p = relative page address (y)
		//fpos = ioffset+(p*rowbytes);
		//fseek(f, fpos, SEEK_SET);
		for (uint16_t c = 0; c < show_width; c++) 
		{
			// c = relative column address (x)
			read(client_to_server, input_buf, 3);

			// B buf[0]
			// G buf[1]
			// R buf[2]
			// 18bit color mode
			screen_buffer[pos_in_local_buf++] = input_buf[2];
			screen_buffer[pos_in_local_buf++] = input_buf[1];
			screen_buffer[pos_in_local_buf++] = input_buf[0];
			pos_in_local_buf++;
			//lcd_colorRGB(buf[2], buf[1], buf[0]);
		}
		if(tail > 0)
		{
			if(!skip_read(tail))		
			{
				printf("\nError reading BMP!\n");
				return;
			}
		}
	}
	//fclose(fp);
	printf("\n");
	show_part(0, 0, show_width, show_height);
}

void export_png()
{
	printf("Export screen to PNG '/tmp/export.png'...\n");
	cairo_surface_write_to_png (surface, "/tmp/export.png");
	printf("Export success\n");

}


dk_control *find_control(uint16_t id)
{
	for(uint8_t i = 0; i <= last_control; i++)
	{
		if(dk_controls[i].id == id)
			return &dk_controls[i];
	}
	return NULL;
}

char *strmbtok ( char *input, char *delimit, const char *openblock, const char *closeblock) {
    static char *token = NULL;
    char *lead = NULL;
    char *block = NULL;
    int iBlock = 0;
    int iBlockIndex = 0;

    if ( input != NULL) {
        token = input;
        lead = input;
    }
    else {
        lead = token;
        if ( *token == '\0') {
            lead = NULL;
        }
    }

    while ( *token != '\0') {
        if ( iBlock) {
            if ( closeblock[iBlockIndex] == *token) {
                iBlock = 0;
            }
            token++;
            continue;
        }
        if ( ( block = strchr ( openblock, *token)) != NULL) {
            iBlock = 1;
            iBlockIndex = block - openblock;
            token++;
            continue;
        }
        if ( strchr ( delimit, *token) != NULL) {
            *token = '\0';
            token++;
            break;
        }
        token++;
    }
    return lead;
}


// if max = 0 - unlimited
uint get_uint(uint max, bool *is_error)
{
	char* endptr;
	*is_error = false;
	errno = 0;
	char *pch = strmbtok (NULL, " ", acOpen, acClose);
	uint val = strtoumax(pch, &endptr, 10);
	if(errno != 0)
	{
		*is_error = true;
		return 0;
	}
	if(max > 0 && val > max)
	{
		val = max;
	}
	return val;
}
/*
// types: 0 - string, 10 - uint8_t, 11 - uint16_t, 20 - double
void read_command_params(uint8_t cnt, char *params_name[], uint8_t params_type[], void *params_vals[])
{
	uint8_t ui8;
	int rcnt;
	my_read_count(client_to_server, input_buf, MYBUF - 1, &rcnt);
	if(rcnt == 0)
		return;
	input_buf[rcnt] = 0;
	bool is_error;
	char *pch = strmbtok ( input_buf, " ", acOpen, acClose);
	while (pch != NULL)
	{
		for(uint8_t i = 0; i < cnt; i++)
		{
			if(strcmp(pch, params_name[i]) != 0)
				continue;
			pch = strmbtok (NULL, " ", acOpen, acClose);
			switch(params_type[i])
			{
				case 0:
					params_vals[i] = pch;
					
					break;
				case 10:
					ui8 = get_uint(0, &is_error);
					if(is_error)
						return;
					*params_vals[i] = ui8;
					break;
			}
		}
		pch = strmbtok (NULL, " ", acOpen, acClose);
	}
}*/

// sample: line w 3 x1 100 y1 300 x2 200 y2 200 r 45 g 55 b 255
// w - strike width
// r, g, b - color, default - black
void draw_line()
{
	uint16_t x1 = 0, x2 = 0, y1 = 0, y2 = 0, w = 1;
	//uint8_t r = 0, g = 0, b = 0;
	hex_color_t color;
	
	int rcnt;
	my_read_count(client_to_server, input_buf, MYBUF - 1, &rcnt);
	if(rcnt == 0)
		return;
	input_buf[rcnt] = 0;
	bool is_error;
	char *pch = strmbtok ( input_buf, " ", acOpen, acClose);
	while (pch != NULL)
	{
		if(strcmp(pch, "w") == 0)
		{
			w = get_uint(0, &is_error);
			if(is_error)
				return;
		}
		else if(strcmp(pch, "x1") == 0)
		{
			x1 = get_uint(LCD_WIDTH - 1, &is_error);
			if(is_error)
				return;
		}
		else if(strcmp(pch, "y1") == 0)
		{
			y1 = get_uint(LCD_HEIGHT - 1, &is_error);
			if(is_error)
				return;
		}
		else if(strcmp(pch, "x2") == 0)
		{
			x2 = get_uint(LCD_WIDTH - 1, &is_error);
			if(is_error)
				return;
		}
		else if(strcmp(pch, "y2") == 0)
		{
			y2 = get_uint(LCD_HEIGHT - 1, &is_error);
			if(is_error)
				return;
		}
		else if(strcmp(pch, "r") == 0)
		{
			color.r = get_uint(255, &is_error);
			if(is_error)
				return;
		}
		else if(strcmp(pch, "g") == 0)
		{
			color.g = get_uint(255, &is_error);
			if(is_error)
				return;
		}
		else if(strcmp(pch, "b") == 0)
		{
			color.b = get_uint(255, &is_error);
			if(is_error)
				return;
		}
		pch = strmbtok (NULL, " ", acOpen, acClose);
	}
	printf("line w = %u, x1 = %u, y1 = %u, x2 = %u, y2 = %u, r = %u, g = %u, b = %u\n",
		w, x1, y1, x2, y2, color.r, color.g, color.b);
	cairo_line(cr, w, x1, y1, x2, y2, color);
	uint16_t left, top, width, height;
	if(x1 <= x2)
	{
		left = x1;
		width = x2 - x1;
	}
	else
	{
		left = x2;
		width = x1 - x2;
	}
	if(y1 <= y2)
	{
		top = y1;
		height = y2 - y1;
	}
	else
	{
		top = y2;
		height = y1 - y2;
	}
	printf("show part left = %u, top = %u, width = %u, height = %u\n", left, top, width, height);
	show_part(left, top, width, height);
//	show_part(0, 0, LCD_WIDTH, LCD_HEIGHT);
}


// sample: labl x 100 y 300 text "hello world" r 45 g 55 b 255
// w - strike width
// r, g, b - color, default - black
void draw_label()
{
    
	//uint16_t x1 = 0, x2 = 0, y1 = 0, y2 = 0, w = 1;
	//uint8_t r = 0, g = 0, b = 0;
	
	int rcnt;
	my_read_count(client_to_server, input_buf, MYBUF - 1, &rcnt);
	if(rcnt == 0)
		return;
	input_buf[rcnt] = 0;
	char *tok = strmbtok ( input_buf, " ", acOpen, acClose);
    printf ( "%s\n", tok);
    while ( ( tok = strmbtok ( NULL, " ", acOpen, acClose)) != NULL) {
        printf ( "%s\n", tok);
    }
}

/*// echo -e 'dlbl\x20\x00\x60\x00\x50\x00\x11\x12\x13\x05\x00Hello' > /tmp/kedei_lcd_in
void draw_d_label()
{
	struct __attribute__((__packed__)) 
	{
		uint16_t font_size;
		uint16_t x;
		uint16_t y;
		uint8_t r;
		uint8_t g;
		uint8_t b;
		uint16_t text_len;
	}d_label_data;
	
	int rcnt;
	my_read_count(client_to_server, &d_label_data, sizeof(d_label_data), &rcnt);
	if(rcnt == 0)
		return;
		
	printf("struct size = %d readed = %d raw text_len = %u\n", sizeof(d_label_data), rcnt, d_label_data.text_len);
	if(d_label_data.text_len > MYBUF - 1)
		d_label_data.text_len = MYBUF - 1;
	my_read_count(client_to_server, input_buf, d_label_data.text_len, &rcnt);
	if(rcnt == 0)
		return;
	input_buf[d_label_data.text_len] = 0;
	printf("font_size = %u, x = %u, y = %u, r = %u, g = %u, b = %u, text_len = %u, text = %s\n",
		d_label_data.font_size, d_label_data.x, d_label_data.y, d_label_data.r, d_label_data.g, d_label_data.b, d_label_data.text_len,
		input_buf);	
	control_label(cr, d_label_data.x, d_label_data.y, d_label_data.font_size, input_buf, d_label_data.r, d_label_data.g, d_label_data.b);
	show_part(0, 0, LCD_WIDTH, LCD_HEIGHT);
}*/


dk_control *add_control(uint16_t id, control_types type, uint16_t left, uint16_t top,
	uint16_t width, uint16_t height,
	void *control_data)
{
	if(last_control >= MAX_CONTROL)
	{
		printf("Control limit!\n");
		free(control_data);
		return NULL;
	}
	
	if(find_control(id) != NULL)
	{
		printf("Control with id = %d already exist!\n", id);
		free(control_data);
		return NULL;
	}
		
	last_control++;
	dk_controls[last_control].id = id;
	dk_controls[last_control].type = type;
	dk_controls[last_control].left = left;
	dk_controls[last_control].top = top;
	dk_controls[last_control].right = left + width;
	dk_controls[last_control].bottom = top + height;
	dk_controls[last_control].control_data = control_data;
	
	show_control(cr, &dk_controls[last_control]);
	show_part(left, top, width, height);
	return &dk_controls[last_control];
}

//              id      fsize   x       y       width   height  r   g   b
// echo -e 'dlbl\x02\x00\x20\x00\x60\x00\x80\x00\x60\x00\x24\x00\x11\x12\x80\x05\x00Logos' > /tmp/kedei_lcd_in
void draw_d_label()
{
	printf("draw_d_text_box\n");
	struct __attribute__((__packed__)) d_text_box_data_tag
	{
		uint16_t id;
		uint16_t font_size;
		uint16_t x;
		uint16_t y;
		uint16_t width;
		uint16_t height;
		uint8_t r;
		uint8_t g;
		uint8_t b;
		uint16_t text_len;
	}d_text_box_data;
	
	
	
	//struct __attribute__((__packed__)) d_text_box_data_tag *d_text_box_data = (struct __attribute__((__packed__)) d_text_box_data_tag*)malloc(sizeof(struct __attribute__((__packed__)) d_text_box_data_tag));
	
	int rcnt;
	my_read_count(client_to_server, &d_text_box_data, sizeof(d_text_box_data), &rcnt);
	if(rcnt == 0)
		return;
	if(d_text_box_data.text_len > 0)
	{
		if(d_text_box_data.text_len > MYBUF - 1)
			d_text_box_data.text_len = MYBUF - 1;
		my_read_count(client_to_server, input_buf, d_text_box_data.text_len, &rcnt);
		if(rcnt == 0)
			return;
		input_buf[d_text_box_data.text_len] = 0;
	}
	
	printf("id = %u, font_size = %u, x = %u, y = %u, width = %u, height = %u, r = %u, g = %u, b = %u, text_len = %u, text = %s\n",
		d_text_box_data.id, d_text_box_data.font_size, d_text_box_data.x, d_text_box_data.y, d_text_box_data.width, d_text_box_data.height, 
		d_text_box_data.r, d_text_box_data.g, d_text_box_data.b, d_text_box_data.text_len,
		input_buf);	
	
	struct label_data_tag *text_box_data = (struct label_data_tag*)malloc(sizeof(struct label_data_tag));
	text_box_data->font_size = d_text_box_data.font_size;
	text_box_data->color.r = d_text_box_data.r;
	text_box_data->color.g = d_text_box_data.g;
	text_box_data->color.b = d_text_box_data.b;
	
	uint16_t slen = strlen(input_buf);
	if(slen > 0)
	{
		text_box_data->text = malloc(slen + 1);
		strcpy(text_box_data->text, input_buf);
	}
	else
	{
		text_box_data->text = NULL;
	}
	
	add_control(d_text_box_data.id, CT_LABEL, d_text_box_data.x, d_text_box_data.y, d_text_box_data.width, d_text_box_data.height, text_box_data);
}

//              id      fsize   x       y       width   height  r   g   b
// echo -e 'dtbx\x01\x00\x20\x00\x60\x00\x50\x00\x60\x00\x24\x00\x11\x80\x13\x05\x00Hello' > /tmp/kedei_lcd_in
void draw_d_text_box()
{
	printf("draw_d_text_box\n");
	struct __attribute__((__packed__)) d_text_box_data_tag
	{
		uint16_t id;
		uint16_t font_size;
		uint16_t x;
		uint16_t y;
		uint16_t width;
		uint16_t height;
		uint8_t r;
		uint8_t g;
		uint8_t b;
		uint16_t text_len;
	}d_text_box_data;
	
	
	
	//struct __attribute__((__packed__)) d_text_box_data_tag *d_text_box_data = (struct __attribute__((__packed__)) d_text_box_data_tag*)malloc(sizeof(struct __attribute__((__packed__)) d_text_box_data_tag));
	
	int rcnt;
	my_read_count(client_to_server, &d_text_box_data, sizeof(d_text_box_data), &rcnt);
	if(rcnt == 0)
		return;
	if(d_text_box_data.text_len > 0)
	{
		if(d_text_box_data.text_len > MYBUF - 1)
			d_text_box_data.text_len = MYBUF - 1;
		my_read_count(client_to_server, input_buf, d_text_box_data.text_len, &rcnt);
		if(rcnt == 0)
			return;
		input_buf[d_text_box_data.text_len] = 0;
	}
	
	printf("id = %u, font_size = %u, x = %u, y = %u, width = %u, height = %u, r = %u, g = %u, b = %u, text_len = %u, text = %s\n",
		d_text_box_data.id, d_text_box_data.font_size, d_text_box_data.x, d_text_box_data.y, d_text_box_data.width, d_text_box_data.height, 
		d_text_box_data.r, d_text_box_data.g, d_text_box_data.b, d_text_box_data.text_len,
		input_buf);	
	
	struct text_box_data_tag *text_box_data = (struct text_box_data_tag*)malloc(sizeof(struct text_box_data_tag));
	text_box_data->font_size = d_text_box_data.font_size;
	text_box_data->color.r = d_text_box_data.r;
	text_box_data->color.g = d_text_box_data.g;
	text_box_data->color.b = d_text_box_data.b;
	
	uint16_t slen = strlen(input_buf);
	if(slen > 0)
	{
		text_box_data->text = malloc(slen + 1);
		strcpy(text_box_data->text, input_buf);
	}
	else
	{
		text_box_data->text = NULL;
	}
	
	add_control(d_text_box_data.id, CT_TEXT_BOX, d_text_box_data.x, d_text_box_data.y, d_text_box_data.width, d_text_box_data.height, text_box_data);
}


//              id     
// echo -e 'dstx\x01\x00\x05\x00World' > /tmp/kedei_lcd_in
void d_set_text()
{
	printf("set text\n");
	struct __attribute__((__packed__))
	{
		uint16_t id;
		uint16_t text_len;
	}d_set_text_data;
	
	int rcnt;
	my_read_count(client_to_server, &d_set_text_data, sizeof(d_set_text_data), &rcnt);
	if(rcnt == 0)
		return;
	char *new_text = NULL;
	if(d_set_text_data.text_len > 0)
	{
		if(d_set_text_data.text_len > MYBUF - 1)
			d_set_text_data.text_len = MYBUF - 1;
		my_read_count(client_to_server, input_buf, d_set_text_data.text_len, &rcnt);
		if(rcnt == 0)
			return;
		input_buf[d_set_text_data.text_len] = 0;
		new_text = input_buf;
	}
	
	
	dk_control *control = find_control(d_set_text_data.id);
	if(control == NULL)
	{
		printf("control not found!");
		return;
	}
	
	set_text(cr, control, new_text);
	show_part(control->left, control->top, control->right - control->left, control->bottom - control->top);
}


/*
 * image_type:
 * 	0 - png
 *
 * scale_type:
 * 	0 - no scale
 *  1 - fit width
 *  2 - fit height
 *	3 - fit on max dimension
 * 	4 - fit on min dimension
 *  5 - stretch

 */
void draw_d_image()
{
	printf("static image\n");
	struct __attribute__((__packed__))
	{
		uint16_t id;
		uint16_t x;
		uint16_t y;
		uint16_t width;
		uint16_t height;
		uint8_t image_type;
		uint8_t scale_type;
		uint32_t image_len;
	}d_image_data;

	int rcnt;
	my_read_count(client_to_server, &d_image_data, sizeof(d_image_data), &rcnt);
	if(rcnt < sizeof(d_image_data))
	{
		printf("Not need data! read: %d, need: %d\n", rcnt, sizeof(d_image_data));
		return;
	}
	printf("Image size = %u\n", d_image_data.image_len);
	uint8_t *image_src = NULL;
	if(d_image_data.image_len > 0)
	{
		image_src = malloc(d_image_data.image_len);
		if(image_src == NULL)
			return;
			
		bool all_read = my_read_count(client_to_server, image_src, d_image_data.image_len, &rcnt);
		if(rcnt == 0 || !all_read)
			return;
		printf("Read image success!\n");
	}

	struct dk_image_data_tag *dk_image_data = (struct dk_image_data_tag*)malloc(sizeof(struct dk_image_data_tag));
	dk_image_data->image_type = d_image_data.image_type;
	dk_image_data->scale_type = d_image_data.scale_type;
	dk_image_data->image_len = d_image_data.image_len;
	dk_image_data->image_data = image_src;

	if(!add_control(d_image_data.id, CT_STATIC_IMAGE, d_image_data.x, d_image_data.y,
		d_image_data.width, d_image_data.height, dk_image_data))
	{
		free(image_src);
	}		
	
	//show_part(0, 0, LCD_WIDTH, LCD_HEIGHT);
}
	
// return: true - exit
bool process_signature(uint8_t sig[])
{
	if(compare_signature(sig, "exit"))
	{
		return true;
		close(client_to_server);
	}
	// export PNG
	if(compare_signature(sig, "expo"))
	{
		export_png();
		return false;
	}
	// Line 
	if(compare_signature(sig, "line"))
	{
		draw_line();
		return false;
	}
	// Label 
	if(compare_signature(sig, "labl"))
	{
		draw_label();
		return false;
	}
	if(compare_signature(sig, "dlbl"))
	{
		draw_d_label();
		return false;
	}
	// text box
	if(compare_signature(sig, "dtbx"))
	{
		draw_d_text_box();
		return false;
	}
	
	// commands
	// set text
	if(compare_signature(sig, "dstx"))
	{
		d_set_text();
		return false;
	}
	
	// show image
	if(compare_signature(sig, "dimg"))
	{
		draw_d_image();
		return false;
	}
	
	
	//int read_buf_pos = 0;
	if(compare_signature(sig, "BM"))
	{
		printf("BMP file detected!\n");
		receive_bmp_file(sig[2], sig[3]);
		return false;
	}
	else
	{
		printf("Unknown signature! %s\n", sig);
	}
	uint16_t allRead = 0;
	/*while (1)
	{
	   
		unsigned short readed = read(client_to_server, &buf[read_buf_pos], MYBUF);
		read_buf_pos = 0;
		allRead += readed;
		if(readed == 0)
		{
			printf("Read 0 bytes!. Close!\n");
			break;
		}

		printf("Received: readed = %u last byte = 0x%2X\n", readed, buf[readed - 1]);
		
	  
		/ *if (strcmp("",buf)!=0)
		{
			printf("Received: %s (len = %d)\n", buf, strlen(buf));
			//printf("Sending back...\n");
			//write(server_to_client,buf,BUFSIZ);
		}* /

		// clean buf from any data 
		memset(buf, 0, sizeof(buf));
	}*/
	printf("All Read %u bytes!\n", allRead);
	return false;
}


int fifo_loop()
{
	printf("Server ON..............bufer = %d\n", MYBUF);
	char *client_to_server_name = "/tmp/kedei_lcd_in";

	//int server_to_client;
	char *server_to_client_name = "/tmp/kedei_lcd_out";


	/* create the FIFO (named pipe) */
	printf("Creating FIFO %s ...\n", client_to_server_name);
	if(mkfifo(client_to_server_name, 0666) < 0)
	{
	   printf("Error creating FIFO %s\n", client_to_server_name);
	   //return 1;
	}
	printf("Creating FIFO %s ...\n", server_to_client_name);
	if(mkfifo(server_to_client_name, 0666) < 0)
	{
	   printf("Error creating FIFO %s\n", server_to_client_name);
	   //return 1;
	}

	/* open, read, and display the message from the FIFO */
	
	/*printf("Opening FIFO %s ...\n", myfifo2);
	server_to_client = open(myfifo2, O_WRONLY | O_NONBLOCK);
	if(server_to_client < 0)
	{
	   printf("Error open FIFO %s\n", myfifo2);
	   perror("open");
	   return 1;
	}*/
	uint8_t signature[SIGNATURE_SIZE];
	printf("Server ON.\n");
	while(1)
	{
		printf("Opening FIFO %s adn waiting for clients ...\n", client_to_server_name);
		client_to_server = open(client_to_server_name, O_RDONLY);//
		if(client_to_server < 0)
		{
			printf("Error open FIFO %s\n", client_to_server_name);
			return 1;
		}
		do
		{
			unsigned short sig_readed = read(client_to_server, signature, SIGNATURE_SIZE);
			if(sig_readed == 0)
			{
				// Fifo closed
				break;
			}
			if(sig_readed < SIGNATURE_SIZE)
			{
				printf("Error! Signature need at leadt %d bytes!\n", SIGNATURE_SIZE);
				//close(client_to_server);
				continue;
			}
			bool prosess_res;
			
			pthread_mutex_lock(&lock_draw);
			prosess_res = process_signature(signature);
			pthread_mutex_unlock(&lock_draw);
			if(prosess_res)
			{
				break;
			}
		}while(true);
		close(client_to_server);
		
	}

   printf("Server OFF.\n");
   //close(server_to_client);

   unlink(client_to_server_name);
   unlink(server_to_client_name);
   return 0;
}

void create_time_thread(dk_control *time_control)
{
	int err = pthread_create(&(tid[0]), NULL, &doTimeShow, time_control);
    if (err != 0)
        printf("\ncan't create thread :[%s]", strerror(err));
    else
        printf("\n Thread created successfully\n");
}


#define SETTING_TOUCH_CALIB_GROUP "touchcalibration"
#define SETTING_X_OFFSET "x_offset"
#define SETTING_Y_OFFSET "y_offset"
#define SETTING_X_COEF "x_coef"
#define SETTING_Y_COEF "y_coef"
#define SETTING_CALIBRATED "calibrated"

bool load_settings()
{
	struct passwd *pw = getpwuid(getuid());

	const char *homedir = pw->pw_dir;
	printf("homedir = %s\n", homedir);
	char *fullpath = malloc(strlen(homedir) + strlen(SETTING_FILE) + 1);
	if (fullpath == NULL)
	{
		printf("Error allocate memory for setting path!\n");
		return false;
	}
	sprintf(fullpath, "%s%s", homedir, SETTING_FILE);
	printf("Full settings path = %s\n", fullpath);

	config_t cfg; 
	config_setting_t *setting;//, *group *root, ;//, *array;
	config_init(&cfg); /* обязательная инициализация */

	if( access( fullpath, F_OK ) != -1 )
	{
		// file exists
		if(! config_read_file(&cfg, fullpath))
		{
			fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg),
					config_error_line(&cfg), config_error_text(&cfg));
			config_destroy(&cfg);
			free(fullpath);
			return(false);
		}

		setting = config_lookup(&cfg, SETTING_TOUCH_CALIB_GROUP "." SETTING_CALIBRATED);
		if(setting != NULL)
		{
			touch_calibrated = config_setting_get_bool(setting);
		}
		if(touch_calibrated)
		{
			setting = config_lookup(&cfg, SETTING_TOUCH_CALIB_GROUP "." SETTING_X_OFFSET);
			if(setting != NULL)
			{
				touch_offset_x = config_setting_get_int(setting);
			}
			setting = config_lookup(&cfg, SETTING_TOUCH_CALIB_GROUP "." SETTING_Y_OFFSET);
			if(setting != NULL)
			{
				touch_offset_y = config_setting_get_int(setting);
			}
			setting = config_lookup(&cfg, SETTING_TOUCH_CALIB_GROUP "." SETTING_X_COEF);
			if(setting != NULL)
			{
				touch_scale_x = config_setting_get_float(setting);
			}
			setting = config_lookup(&cfg, SETTING_TOUCH_CALIB_GROUP "." SETTING_Y_COEF);
			if(setting != NULL)
			{
				touch_scale_y = config_setting_get_float(setting);
			}
			
		}
	}
	
	config_destroy(&cfg);
	free(fullpath);
	return true;
}

void save_settings()
{
	struct passwd *pw = getpwuid(getuid());

	const char *homedir = pw->pw_dir;
	printf("homedir = %s\n", homedir);
	char *fullpath = malloc(strlen(homedir) + strlen(SETTING_FILE) + 1);
	if (fullpath == NULL)
	{
		printf("Error allocate memory for setting path!\n");
		return;
	}
	sprintf(fullpath, "%s%s", homedir, SETTING_FILE);
	printf("Full settings path = %s\n", fullpath);

	config_t cfg; 
	config_setting_t *root, *setting, *group;//, *array;
	config_init(&cfg); /* обязательная инициализация */

	// create new settings
	root = config_root_setting(&cfg);
/* Add some settings to the configuration. */
	group = config_setting_add(root, SETTING_TOUCH_CALIB_GROUP, CONFIG_TYPE_GROUP);

	setting = config_setting_add(group, SETTING_X_OFFSET, CONFIG_TYPE_INT);
	config_setting_set_int(setting, touch_offset_x);

	setting = config_setting_add(group, SETTING_Y_OFFSET, CONFIG_TYPE_INT);
	config_setting_set_int(setting, touch_offset_y);

	setting = config_setting_add(group, SETTING_X_COEF, CONFIG_TYPE_FLOAT);
	config_setting_set_float(setting, touch_scale_x);

	setting = config_setting_add(group, SETTING_Y_COEF, CONFIG_TYPE_FLOAT);
	config_setting_set_float(setting, touch_scale_y);

	setting = config_setting_add(group, SETTING_CALIBRATED, CONFIG_TYPE_BOOL);
	config_setting_set_bool(setting, touch_calibrated);

	// Write out the new configuration. 
	if(! config_write_file(&cfg, fullpath))
	{
		fprintf(stderr, "Error while writing file.\n");
	}
	config_destroy(&cfg);
	free(fullpath);
	return;
}

void draw_calib_cross(uint16_t x, uint16_t y)
{
	cairo_set_source_rgb(cr, 0, 0, 0);
	cairo_set_line_width(cr, 1);
	
	cairo_move_to(cr, x - 20, y);
    cairo_line_to(cr, x + 20, y);

	cairo_move_to(cr, x, y - 20);
    cairo_line_to(cr, x, y + 20);

    cairo_stroke(cr);
    
	show_part(x - 20, y - 20, 40, 40);
}

void draw_calib_circle(uint16_t x, uint16_t y)
{
	cairo_set_line_width(cr, 9);  
	cairo_set_source_rgb(cr, 0.69, 0.19, 0);
	
	cairo_arc(cr, x, y, 20, 0, 2 * M_PI);

    cairo_fill (cr);
    
	show_part(x - 20, y - 20, 40, 40);
}

#define CALIB_ERR_DIST 100
bool calibrate_touch()
{
	int lt_x, lt_y,
		rt_x, rt_y,
		rb_x, rb_y,
		lb_x, lb_y;
		
	cairo_clear_all(cr);
	show_part(0, 0, LCD_WIDTH, LCD_HEIGHT);
	
	printf("Calibrating touch...\n");
	draw_text_in_rect(cr, 30, 130, 150, 240, 36, get_std_color(COL_BLACK), get_std_color(COL_BG_COLOR), "Calibrating...");
	show_part(130, 150, 240, 36);
	
	// laft top point 30 30
	draw_calib_cross(30, 30);
	touch_raw_x = 0;
	touch_raw_y = 0;
    while(touch_raw_x == 0 || touch_raw_y == 0)
    {
		sleep(1);
	}
	lt_x = touch_raw_x;
	lt_y = touch_raw_y;
	printf("Top left: x = %d y = %d \n", lt_x, lt_y);
	draw_calib_circle(30, 30);
	
	// right top point
	draw_calib_cross(LCD_WIDTH - 30, 30);
	touch_raw_x = 0;
	touch_raw_y = 0;
    while(touch_raw_x == 0 || touch_raw_y == 0)
    {
		sleep(1);
	}
	rt_x = touch_raw_x;
	rt_y = touch_raw_y;
	printf("Top right: x = %d y = %d \n", rt_x, rt_y);
	if(abs(lt_y - rt_y) > CALIB_ERR_DIST)
	{
		printf("Diff y!\n");
		return false;
	}

	draw_calib_circle(LCD_WIDTH - 30, 30);
   
	// right bottom point
	draw_calib_cross(LCD_WIDTH - 30, LCD_HEIGHT - 30);
	touch_raw_x = 0;
	touch_raw_y = 0;
    while(touch_raw_x == 0 || touch_raw_y == 0)
    {
		sleep(1);
	}
	rb_x = touch_raw_x;
	rb_y = touch_raw_y;
	printf("Bottom right: x = %d y = %d \n", rb_x, rb_y);
	if(abs(rt_x - rb_x) > CALIB_ERR_DIST)
	{
		printf("Diff x!\n");
		return false;
	}

	draw_calib_circle(LCD_WIDTH - 30, LCD_HEIGHT - 30);
	   
	// left bottom point
	draw_calib_cross(30, LCD_HEIGHT - 30);
	touch_raw_x = 0;
	touch_raw_y = 0;
    while(touch_raw_x == 0 || touch_raw_y == 0)
    {
		sleep(1);
	}
	lb_x = touch_raw_x;
	lb_y = touch_raw_y;
	printf("Bottom left: x = %d y = %d \n", lb_x, lb_y);
	if(abs(lt_x - lb_x) > CALIB_ERR_DIST
		|| abs(lb_y - rb_y) > CALIB_ERR_DIST)
	{
		printf("Diff x or y!\n");
		return false;
	}
	draw_calib_circle(30, LCD_HEIGHT - 30);

	int dist_x = LCD_WIDTH - 60;
	int dist_y = LCD_HEIGHT - 60;

	double mid_left_x = (lt_x + lb_x) / 2.0;
	double mid_right_x = (rt_x + rb_x) / 2.0;
	
	double mid_top_y = (lt_y + rt_y) / 2.0;
	double mid_bot_y = (lb_y + rb_y) / 2.0;

	double tmp_scale_x = (double)dist_x / (mid_right_x - mid_left_x);
	double tmp_scale_y = (double)dist_y / (mid_bot_y - mid_top_y);

	if(fabs(tmp_scale_x) < 0.001 || fabs(tmp_scale_y) < 0.001)
	{
		printf("Scale error x = %f y = %f", tmp_scale_x, tmp_scale_y);
		return false;
	}

	touch_scale_x = tmp_scale_x;
	touch_scale_y = tmp_scale_y;

	touch_offset_x = mid_left_x - 30 / touch_scale_x;
	touch_offset_y = mid_top_y - 30 / touch_scale_y;
	
//int touch_offset_x = 0, touch_offset_y = 0;
//double touch_scale_x = 1, touch_scale_y = 1;
	
	printf("Calibrating complete!Offset X = %d Y = %d  Scale X = %f Y = %f\n",
		touch_offset_x, touch_offset_y, touch_scale_x, touch_scale_y);
	touch_calibrated = true;
	save_settings();
	// testing
	draw_text_in_rect(cr, 30, 130, 120, 240, 36, get_std_color(COL_BLACK), get_std_color(COL_BG_COLOR), "Touch test...");
	draw_text_in_rect(cr, 30, 130, 160, 240, 36, get_std_color(COL_BLACK), get_std_color(COL_BG_COLOR), "For exit press OK");

	draw_text_in_rect(cr, 30, 200, 200, 60, 36, get_std_color(COL_BLACK), get_std_color(COL_BG_COLOR), "OK");

	cairo_set_source_rgb (cr, 0, 1, 0);
	cairo_set_line_width(cr, 2);
	cairo_rectangle (cr, 200, 200, 60, 36);
	cairo_stroke (cr);
	show_part(130, 120, 240, 150);

	

	while(true)
	{
		touch_x = 1000;
		touch_y = 1000;
		while(touch_x == 1000 || touch_y == 1000)
		{
			sleep(1);
		}
		if(touch_x >= 200 && touch_x < 200 + 60
			&& touch_y >= 200 && touch_y < 200 + 36)
			return true;
		draw_calib_circle(touch_x, touch_y);
	}
	
    return true;
}

int main(int argc,char *argv[]) 
{
	//cairo_test();
	//return 0;
	
	
	uint8_t initRotation = 0;
	int aflag = 0;
	int bflag = 0;
	char *cvalue = NULL;
	char *bmpFile = NULL;
	int index;
	int c;

  opterr = 0;

  while ((c = getopt (argc, argv, "ab:c:r:")) != -1)
    switch (c)
      {
      case 'a':
        aflag = 1;
        break;
      case 'b':
        bmpFile = optarg;
        break;
      case 'c':
        cvalue = optarg;
        break;
      case 'r':
		if(strcmp(optarg, "90") == 0)
		{
			initRotation = 1;
		}
		else if(strcmp(optarg, "180") == 0)
		{
			initRotation = 2;
		}
		else if(strcmp(optarg, "270") == 0)
		{
			initRotation = 3;
		}
		else
		{
			fprintf (stderr, "Incorrect rotation '%s'!\n", optarg);
		}
		break;
      case '?':
        if (optopt == 'c')
          fprintf (stderr, "Option -%c requires an argument.\n", optopt);
        else if (isprint (optopt))
          fprintf (stderr, "Unknown option `-%c'.\n", optopt);
        else
          fprintf (stderr,
                   "Unknown option character `\\x%x'.\n",
                   optopt);
        return 1;
      default:
        abort ();
      }

	printf ("rotation = %d, aflag = %d, bflag = %d, cvalue = %s, bmpFile = %s \n",
          initRotation, aflag, bflag, cvalue, bmpFile);

	for (index = optind; index < argc; index++)
		printf ("Non-option argument %s\n", argv[index]);


	

	// get settings
	if(!load_settings())
	{
		return EXIT_FAILURE;
	}
	//return EXIT_SUCCESS;

	


	printf ("Open LCD\n");
	//delayms(3000);
	if(lcd_open() < 0)
	{
		fprintf (stderr, "Error initialise GPIO!\nNeed stop daemon:\n  sudo killall pigpiod");
		return 1;
	}

	
	
	create_sensor_thread();

	
	
	printf ("Init LCD\n");
	//delayms(3000);
	lcd_init(initRotation);
	
	//printf ("Black LCD\n");
	//delayms(3000);
	//lcd_fill(0); //black out the screen.
	//hex_color_t BG_COLOR =  { 0xd4, 0xd0, 0xc8 };

    surface = cairo_image_surface_create_for_data (screen_buffer, CAIRO_FORMAT_RGB24,
						   LCD_WIDTH, LCD_HEIGHT, STRIDE);
    cr = cairo_create (surface);
	

	if(!touch_calibrated)
		while(!calibrate_touch());
		

	cairo_clear_all(cr);
	show_part(0, 0, LCD_WIDTH, LCD_HEIGHT);
	

	//return EXIT_SUCCESS;

	if (pthread_mutex_init(&lock_draw, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }
	struct label_data_tag *text_box_data = (struct label_data_tag*)malloc(sizeof(struct label_data_tag));
	text_box_data->font_size = 32;
	text_box_data->color.r = 40;
	text_box_data->color.g = 0;
	text_box_data->color.b = 0;
	text_box_data->text = NULL;// malloc(3);
	//strcpy(text_box_data->text, "Gt");
	//text_box_data->text[2] = 0;
	
	dk_control *time_control = add_control(255, CT_LABEL, 200, 200, 150, 36, text_box_data);
	

	create_time_thread(time_control);








	
	fifo_loop();

	/*if(bmpFile != NULL)
	{
		lcd_img(bmpFile, 0, 0);
		
		fifo_loop();
		lcd_close();
		
		return 0;
	}*/
	cairo_destroy (cr);

    cairo_surface_destroy (surface);
    
    lcd_close();

    pthread_mutex_destroy(&lock_draw);
    return 0;
/*
	// 24bit Bitmap only
	lcd_img("kedei_lcd_v50_pi.bmp", 50, 5);
	delayms(500);

	lcd_fillRGB(0xFF, 0x00, 0x00);
	lcd_fillRGB(0x00, 0xFF, 0x00);
	lcd_fillRGB(0xFF, 0xFF, 0x00);
	lcd_fillRGB(0x00, 0x00, 0xFF);
	lcd_fillRGB(0xFF, 0x00, 0xFF);
	lcd_fillRGB(0x00, 0xFF, 0xFF);
	lcd_fillRGB(0xFF, 0xFF, 0xFF);
	lcd_fillRGB(0x00, 0x00, 0x00);

	// 24bit Bitmap only
	lcd_img("kedei_lcd_v50_pi.bmp", 50, 5);
	delayms(500);

	// Demo
	color=0;
	lcd_rotation=0;
	loop();	loop();	loop();	loop();
	loop();	loop();	loop();	loop();
	loop();	loop();	loop();	loop();
	loop();	loop();	loop();	loop();
	loop();

	// 24bit Bitmap only
	lcd_img("kedei_lcd_v50_pi.bmp", 50, 5);

	lcd_close();*/
}


// time thread
void* doTimeShow(void *arg)
{
	
	dk_control *time_control = (dk_control *)arg;
    unsigned long i = 0;
    //pthread_t id = pthread_self();

//    if(pthread_equal(id,tid[0]))
//    {
        printf("\n Time thread start. Control id = %u\n", time_control->id);
//    }
//    else
//    {
//        printf("\n Second thread processing\n");
//    }
	char buf[9];
	buf[8] = 0;
	while(false)//!!!!!!
	{
		time_t t = time(NULL);
		struct tm tm = *localtime(&t);
		sprintf(buf, "%2d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
		pthread_mutex_lock(&lock_draw);
		//printf("Cur time = %s\n", buf);
		set_text(cr, time_control, buf);
		show_part(time_control->left, time_control->top, time_control->right - time_control->left, time_control->bottom - time_control->top);
		pthread_mutex_unlock(&lock_draw);
		
		sleep(1);
		i++;
	}
	printf("Exit thread");
    return NULL;
}

